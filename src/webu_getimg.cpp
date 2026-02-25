/*
 *    This file is part of Motion.
 *
 *    Motion is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Motion is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Motion.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

/*
 * webu_getimg.cpp - Single Frame Image Request Handler
 *
 * This module handles HTTP requests for single JPEG frame snapshots
 * from cameras, providing current, previous, and motion-detected frame
 * images on demand for the web interface.
 *
 */

#include "motion.hpp"
#include "util.hpp"
#include "conf.hpp"
#include "logger.hpp"
#include "camera.hpp"
#include "picture.hpp"
#include "alg_sec.hpp"
#include "webu_getimg.hpp"

/* NOTE:  These run on the camera thread. */

/* Initial the stream context items for the camera */
void webu_getimg_init(cls_camera *cam)
{
    cam->imgs.image_substream = NULL;
    cam->imgs.image_substream_tmp = NULL;

    cam->stream.norm.jpg_sz = 0;
    cam->stream.norm.jpg_data = NULL;
    cam->stream.norm.jpg_cnct = 0;
    cam->stream.norm.all_cnct = 0;
    cam->stream.norm.consumed = true;
    cam->stream.norm.img_data = NULL;

    cam->stream.sub.jpg_sz = 0;
    cam->stream.sub.jpg_data = NULL;
    cam->stream.sub.jpg_cnct = 0;
    cam->stream.sub.all_cnct = 0;
    cam->stream.sub.consumed = true;
    cam->stream.sub.img_data = NULL;

    cam->stream.motion.jpg_sz = 0;
    cam->stream.motion.jpg_data = NULL;
    cam->stream.motion.jpg_cnct = 0;
    cam->stream.motion.all_cnct = 0;
    cam->stream.motion.consumed = true;
    cam->stream.motion.img_data = NULL;

    cam->stream.source.jpg_sz = 0;
    cam->stream.source.jpg_data = NULL;
    cam->stream.source.jpg_cnct = 0;
    cam->stream.source.all_cnct = 0;
    cam->stream.source.consumed = true;
    cam->stream.source.img_data = NULL;

    cam->stream.secondary.jpg_sz = 0;
    cam->stream.secondary.jpg_data = NULL;
    cam->stream.secondary.jpg_cnct = 0;
    cam->stream.secondary.all_cnct = 0;
    cam->stream.secondary.consumed = true;
    cam->stream.secondary.img_data = NULL;

}

/* Free the stream buffers and mutex for shutdown */
void webu_getimg_deinit(cls_camera *cam)
{
    /* NOTE:  This runs on the camera thread. */
    myfree(cam->imgs.image_substream);
    myfree(cam->imgs.image_substream_tmp);

    pthread_mutex_lock(&cam->stream.mutex);
        myfree(cam->stream.norm.jpg_data);
        myfree(cam->stream.sub.jpg_data);
        myfree(cam->stream.motion.jpg_data);
        myfree(cam->stream.source.jpg_data);
        myfree(cam->stream.secondary.jpg_data);

        myfree(cam->stream.norm.img_data) ;
        myfree(cam->stream.sub.img_data) ;
        myfree(cam->stream.motion.img_data) ;
        myfree(cam->stream.source.img_data) ;
        myfree(cam->stream.secondary.img_data) ;
    pthread_mutex_unlock(&cam->stream.mutex);

}

/* Get a normal image from the motion loop and compress it*/
static void webu_getimg_norm(cls_camera *cam)
{
    if ((cam->stream.norm.jpg_cnct == 0) &&
        (cam->stream.norm.all_cnct == 0)) {
        return;
    }

    if (cam->stream.norm.jpg_cnct > 0) {
        if (cam->stream.norm.jpg_data == NULL) {
            cam->stream.norm.jpg_data =(unsigned char*)
                mymalloc((uint)cam->imgs.size_norm);
        }
        if (cam->current_image->image_norm != NULL && cam->stream.norm.consumed) {
            cam->stream.norm.jpg_sz = cam->picture->put_memory(
                cam->stream.norm.jpg_data
                ,cam->imgs.size_norm
                ,cam->current_image->image_norm
                ,cam->cfg->stream_quality
                ,cam->imgs.width
                ,cam->imgs.height);
            cam->stream.norm.consumed = false;
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            double elapsed = (double)(now.tv_sec - cam->stream.norm.last_encode_time.tv_sec) +
                (double)(now.tv_nsec - cam->stream.norm.last_encode_time.tv_nsec) / 1e9;
            if (elapsed > 0.001 && cam->stream.norm.last_encode_time.tv_sec > 0) {
                float instant_fps = 1.0f / (float)elapsed;
                cam->stream.norm.encode_fps = 0.8f * cam->stream.norm.encode_fps + 0.2f * instant_fps;
            }
            cam->stream.norm.last_encode_time = now;
        }
    }
}

/* Get a substream image from the motion loop and compress it*/
static void webu_getimg_sub(cls_camera *cam)
{
    int sub_w, sub_h, subsize, scale;

    if ((cam->stream.sub.jpg_cnct == 0) &&
        (cam->stream.sub.all_cnct == 0)) {
        return;
    }

    if (cam->stream.sub.jpg_cnct > 0) {
        if (cam->stream.sub.jpg_data == NULL) {
            cam->stream.sub.jpg_data = (unsigned char*)
                mymalloc((uint)cam->imgs.size_norm);
        }
        if (cam->current_image->image_norm != NULL && cam->stream.sub.consumed) {
            scale = cam->cfg->substream_scale;

            /* Calculate target dimensions, rounded down to multiple of 16 */
            sub_w = ((cam->imgs.width * scale / 100) / 16) * 16;
            sub_h = ((cam->imgs.height * scale / 100) / 16) * 16;

            /* Validate minimum size; fall back to 50% if too small */
            if (sub_w < 64 || sub_h < 64) {
                sub_w = ((cam->imgs.width / 2) / 16) * 16;
                sub_h = ((cam->imgs.height / 2) / 16) * 16;
                if (sub_w < 64 || sub_h < 64) {
                    /* Camera resolution too low — send full resolution */
                    cam->stream.sub.jpg_sz = cam->picture->put_memory(
                        cam->stream.sub.jpg_data
                        , cam->imgs.size_norm
                        , cam->current_image->image_norm
                        , cam->cfg->substream_quality
                        , cam->imgs.width
                        , cam->imgs.height);
                    cam->stream.sub.consumed = false;
                    return;
                }
                scale = 50;
            }

            subsize = (sub_w * sub_h * 3) / 2;  /* YUV420P */

            /* Allocate substream buffer if needed */
            if (cam->imgs.image_substream == NULL) {
                cam->imgs.image_substream = (unsigned char*)
                    mymalloc((uint)subsize);
            }

            if (scale == 100) {
                /* No scaling — JPEG-encode full-resolution image at substream quality */
                cam->stream.sub.jpg_sz = cam->picture->put_memory(
                    cam->stream.sub.jpg_data
                    , cam->imgs.size_norm
                    , cam->current_image->image_norm
                    , cam->cfg->substream_quality
                    , cam->imgs.width
                    , cam->imgs.height);
            } else if (scale == 50 &&
                       (cam->imgs.width % 16 == 0) &&
                       (cam->imgs.height % 16 == 0)) {
                /* Fast path: 2x nearest-neighbor downsample */
                cam->picture->scale_img(cam->imgs.width
                    , cam->imgs.height
                    , cam->current_image->image_norm
                    , cam->imgs.image_substream);
                cam->stream.sub.jpg_sz = cam->picture->put_memory(
                    cam->stream.sub.jpg_data
                    , subsize
                    , cam->imgs.image_substream
                    , cam->cfg->substream_quality
                    , sub_w, sub_h);
            } else if (scale == 25 &&
                       (cam->imgs.width % 16 == 0) &&
                       (cam->imgs.height % 16 == 0)) {
                /* Fast path: chained 2x downscale (full → half → quarter) */
                int half_w = ((cam->imgs.width / 2) / 16) * 16;
                int half_h = ((cam->imgs.height / 2) / 16) * 16;
                int half_size = (half_w * half_h * 3) / 2;

                if (cam->imgs.image_substream_tmp == NULL) {
                    cam->imgs.image_substream_tmp = (unsigned char*)
                        mymalloc((uint)half_size);
                }

                /* First pass: full → half */
                cam->picture->scale_img(cam->imgs.width
                    , cam->imgs.height
                    , cam->current_image->image_norm
                    , cam->imgs.image_substream_tmp);
                /* Second pass: half → quarter */
                cam->picture->scale_img(half_w, half_h
                    , cam->imgs.image_substream_tmp
                    , cam->imgs.image_substream);

                cam->stream.sub.jpg_sz = cam->picture->put_memory(
                    cam->stream.sub.jpg_data
                    , subsize
                    , cam->imgs.image_substream
                    , cam->cfg->substream_quality
                    , sub_w, sub_h);
            } else {
                /* Arbitrary scale — use FFmpeg sws_scale via util_resize() */
                util_resize(
                    cam->current_image->image_norm
                    , cam->imgs.width, cam->imgs.height
                    , cam->imgs.image_substream
                    , sub_w, sub_h);
                cam->stream.sub.jpg_sz = cam->picture->put_memory(
                    cam->stream.sub.jpg_data
                    , subsize
                    , cam->imgs.image_substream
                    , cam->cfg->substream_quality
                    , sub_w, sub_h);
            }

            cam->stream.sub.consumed = false;
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            double elapsed = (double)(now.tv_sec - cam->stream.sub.last_encode_time.tv_sec) +
                (double)(now.tv_nsec - cam->stream.sub.last_encode_time.tv_nsec) / 1e9;
            if (elapsed > 0.001 && cam->stream.sub.last_encode_time.tv_sec > 0) {
                float instant_fps = 1.0f / (float)elapsed;
                cam->stream.sub.encode_fps = 0.8f * cam->stream.sub.encode_fps + 0.2f * instant_fps;
            }
            cam->stream.sub.last_encode_time = now;
        }
    }

}

/* Get a motion image from the motion loop and compress it*/
static void webu_getimg_motion(cls_camera *cam)
{
    if ((cam->stream.motion.jpg_cnct == 0) &&
        (cam->stream.motion.all_cnct == 0)) {
        return;
    }

    if (cam->stream.motion.jpg_cnct > 0) {
        if (cam->stream.motion.jpg_data == NULL) {
            cam->stream.motion.jpg_data =(unsigned char*)mymalloc((uint)cam->imgs.size_norm);
        }
        if (cam->imgs.image_motion.image_norm != NULL  && cam->stream.motion.consumed) {
            cam->stream.motion.jpg_sz = cam->picture->put_memory(
                cam->stream.motion.jpg_data
                ,cam->imgs.size_norm
                ,cam->imgs.image_motion.image_norm
                ,cam->cfg->stream_quality
                ,cam->imgs.width
                ,cam->imgs.height);
            cam->stream.motion.consumed = false;
        }
    }
}

/* Get a source image from the motion loop and compress it*/
static void webu_getimg_source(cls_camera *cam)
{
    if ((cam->stream.source.jpg_cnct == 0) &&
        (cam->stream.source.all_cnct == 0)) {
        return;
    }

    if (cam->stream.source.jpg_cnct > 0) {
        if (cam->stream.source.jpg_data == NULL) {
            cam->stream.source.jpg_data =(unsigned char*)mymalloc((uint)cam->imgs.size_norm);
        }
        if (cam->imgs.image_virgin != NULL && cam->stream.source.consumed) {
            cam->stream.source.jpg_sz = cam->picture->put_memory(
                cam->stream.source.jpg_data
                ,cam->imgs.size_norm
                ,cam->imgs.image_virgin
                ,cam->cfg->stream_quality
                ,cam->imgs.width
                ,cam->imgs.height);
            cam->stream.source.consumed = false;
        }
    }
}

/* Get a secondary image from the motion loop and compress it*/
static void webu_getimg_secondary(cls_camera *cam)
{
     if ((cam->stream.secondary.jpg_cnct == 0) &&
         (cam->stream.secondary.all_cnct == 0)) {
        return;
    }

    if (cam->stream.secondary.jpg_cnct > 0) {
        if (cam->imgs.size_secondary>0) {
            pthread_mutex_lock(&cam->algsec->mutex);
                if (cam->stream.secondary.jpg_data == NULL) {
                    cam->stream.secondary.jpg_data =(unsigned char*)
                        mymalloc((uint)cam->imgs.size_norm);
                }

                memcpy(cam->stream.secondary.jpg_data
                    , cam->imgs.image_secondary
                    , (uint)cam->imgs.size_secondary);
                cam->stream.secondary.jpg_sz = cam->imgs.size_secondary;
            pthread_mutex_unlock(&cam->algsec->mutex);
        } else {
            myfree(cam->stream.secondary.jpg_data);
        }
    }

}

/* Get image from the motion loop and compress it*/
void webu_getimg_main(cls_camera *cam)
{
    /*This is on the camera thread */
    pthread_mutex_lock(&cam->stream.mutex);
        webu_getimg_norm(cam);
        webu_getimg_sub(cam);
        webu_getimg_motion(cam);
        webu_getimg_source(cam);
        webu_getimg_secondary(cam);
    pthread_mutex_unlock(&cam->stream.mutex);
}
