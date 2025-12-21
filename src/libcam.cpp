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
 *
 */

#include "motion.hpp"
#include "util.hpp"
#include "camera.hpp"
#include "conf.hpp"
#include "logger.hpp"
#include "rotate.hpp"
#include "libcam.hpp"

#ifdef HAVE_LIBCAM

using namespace libcamera;

/* Convert ISO value (100-6400) to AnalogueGain multiplier
 * ISO 100 = gain 1.0, ISO 800 = gain 8.0, ISO 1600 = gain 16.0
 * Note: IMX708 (Pi Camera v3) max analog gain is 16.0 (ISO 1600).
 * Values above ISO 1600 trigger digital gain which adds significant noise
 * and may require reduced framerate (< 10 fps) for stable operation.
 */
static float iso_to_gain(float iso) {
    return iso / 100.0f;
}

void cls_libcam::log_orientation()
{
    #if (LIBCAMVER >= 2000)
        MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "Libcamera Orientation Options:");
        MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  Rotate0");
        MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  Rotate0Mirror");
        MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  Rotate180");
        MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  Rotate180Mirror");
        MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  Rotate90");
        MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  Rotate90Mirror");
        MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  Rotate270");
        MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  Rotate270Mirror");
    #else
        MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO, "Orientation Not available");
    #endif

}

void cls_libcam::log_controls()
{
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "Libcamera Controls:");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AeMeteringMode(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    MeteringCentreWeighted = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    MeteringSpot = 1");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    MeteringMatrix = 2");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    MeteringCustom = 3");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AeConstraintMode(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    ConstraintNormal = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    ConstraintHighlight = 1");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    ConstraintShadows = 2");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    ConstraintCustom = 3");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AeExposureMode(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    ExposureNormal = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    ExposureShort = 1");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    ExposureLong = 2");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    ExposureCustom = 3");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  ExposureValue(float)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  ExposureTime(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AnalogueGain(float)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  Brightness(float)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  Contrast(float)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  Lux(float)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AwbEnable(bool)");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AwbMode(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AwbAuto = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AwbIncandescent = 1");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AwbTungsten = 2");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AwbFluorescent = 3");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AwbIndoor = 4");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AwbDaylight = 5");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AwbCloudy = 6");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AwbCustom = 7");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AwbLocked(bool)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  ColourGains(Pipe delimited)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "     Red | Blue");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  ColourTemperature(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  Saturation(float)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  SensorBlackLevels(Pipe delimited)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "     var1|var2|var3|var4");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  Sharpness(float)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  FocusFoM(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  ColourCorrectionMatrix(Pipe delimited)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "     var1|var2|...|var8|var9");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  ScalerCrop(Pipe delimited)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "     x | y | h | w");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  DigitalGain(float)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  FrameDuration(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  FrameDurationLimits(Pipe delimited)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "     min | max");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  SensorTemperature(float)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  SensorTimestamp(int)");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AfMode(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfModeManual = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfModeAuto = 1");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfModeContinuous = 2");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AfRange(0-2)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfRangeNormal = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfRangeMacro = 1");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfRangeFull = 2");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AfSpeed(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfSpeedNormal = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfSpeedFast = 1");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AfMetering(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfMeteringAuto = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfMeteringWindows = 1");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AfWindows(Pipe delimited)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "     x | y | h | w");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AfTrigger(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfTriggerStart = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfTriggerCancel = 1");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AfPause(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfPauseImmediate = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfPauseDeferred = 1");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfPauseResume = 2");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  LensPosition(float)");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AfState(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfStateIdle = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfStateScanning = 1");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfStateFocused = 2");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfStateFailed = 3");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AfPauseState(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfPauseStateRunning = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfPauseStatePausing = 1");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AfPauseStatePaused = 2");

}

void cls_libcam:: log_draft()
{
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "Libcamera Controls Draft:");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AePrecaptureTrigger(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AePrecaptureTriggerIdle = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AePrecaptureTriggerStart = 1");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AePrecaptureTriggerCancel = 2");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  NoiseReductionMode(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    NoiseReductionModeOff = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    NoiseReductionModeFast = 1");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    NoiseReductionModeHighQuality = 2");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    NoiseReductionModeMinimal = 3");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    NoiseReductionModeZSL = 4");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  ColorCorrectionAberrationMode(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    ColorCorrectionAberrationOff = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    ColorCorrectionAberrationFast = 1");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    ColorCorrectionAberrationHighQuality = 2");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  AwbState(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AwbStateInactive = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AwbStateSearching = 1");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AwbConverged = 2");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    AwbLocked = 3");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  SensorRollingShutterSkew(int)");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  LensShadingMapMode(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    LensShadingMapModeOff = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    LensShadingMapModeOn = 1");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  PipelineDepth(int)");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  MaxLatency(int)");

    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "  TestPatternMode(int)");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    TestPatternModeOff = 0");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    TestPatternModeSolidColor = 1");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    TestPatternModeColorBars = 2");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    TestPatternModeColorBarsFadeToGray = 3");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    TestPatternModePn9 = 4");
    MOTION_SHT(DBG, TYPE_VIDEO, NO_ERRNO, "    TestPatternModeCustom1 = 256");

}

void cls_libcam::start_params()
{
    ctx_params_item *itm;
    int indx;

    params = new ctx_params;
    util_parms_parse(params,"libcam_params", cam->cfg->libcam_params);

    for (indx=0;indx<params->params_cnt;indx++) {
        itm = &params->params_array[indx];
        MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO, "%s : %s"
            ,itm->param_name.c_str(), itm->param_value.c_str());
    }
}

/* Filter out USB/UVC cameras, return only Pi CSI cameras */
std::vector<std::shared_ptr<Camera>> cls_libcam::get_pi_cameras()
{
    std::vector<std::shared_ptr<Camera>> pi_cams;

    for (const auto& cam_item : cam_mgr->cameras()) {
        std::string id = cam_item->id();
        /* Filter out USB cameras (UVC devices)
         * Pi cameras have IDs like: /base/axi/pcie@120000/rp1/i2c@88000/imx708@1a
         * USB cameras have IDs containing "usb" or are UVC devices
         */
        std::string id_lower = id;
        std::transform(id_lower.begin(), id_lower.end(), id_lower.begin(), ::tolower);

        if (id_lower.find("usb") == std::string::npos &&
            id_lower.find("uvc") == std::string::npos) {
            pi_cams.push_back(cam_item);
            MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO
                , "Found Pi camera: %s", id.c_str());
        } else {
            MOTION_LOG(DBG, TYPE_VIDEO, NO_ERRNO
                , "Filtered out USB camera: %s", id.c_str());
        }
    }

    /* Sort for consistent ordering across restarts */
    std::sort(pi_cams.begin(), pi_cams.end(),
        [](const std::shared_ptr<Camera>& a, const std::shared_ptr<Camera>& b) {
            return a->id() < b->id();
        });

    MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO
        , "Found %d Pi camera(s) after filtering", (int)pi_cams.size());

    return pi_cams;
}

/* Find camera by sensor model name (e.g., "imx708", "imx477") */
std::shared_ptr<Camera> cls_libcam::find_camera_by_model(const std::string &model)
{
    std::string model_lower = model;
    std::transform(model_lower.begin(), model_lower.end(), model_lower.begin(), ::tolower);

    for (const auto& cam_item : cam_mgr->cameras()) {
        std::string id = cam_item->id();
        std::string id_lower = id;
        std::transform(id_lower.begin(), id_lower.end(), id_lower.begin(), ::tolower);

        if (id_lower.find(model_lower) != std::string::npos) {
            MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO
                , "Found camera matching model '%s': %s"
                , model.c_str(), id.c_str());
            return cam_item;
        }
    }

    MOTION_LOG(DBG, TYPE_VIDEO, NO_ERRNO
        , "No camera found matching model '%s'", model.c_str());
    return nullptr;
}

int cls_libcam::start_mgr()
{
    int retcd;
    std::string device;
    std::vector<std::shared_ptr<Camera>> pi_cams;

    MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO, "Starting.");

    cam_mgr = std::make_unique<CameraManager>();
    retcd = cam_mgr->start();
    if (retcd != 0) {
        MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
            , "Error starting camera manager.  Return code: %d", retcd);
        return retcd;
    }
    started_mgr = true;

    MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO
        , "cam_mgr started. Total cameras available: %d"
        , (int)cam_mgr->cameras().size());

    /* Get filtered list of Pi cameras (excludes USB/UVC) */
    pi_cams = get_pi_cameras();

    if (pi_cams.empty()) {
        MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
            , "No Pi cameras found. Check camera connection and config.txt");
        return -1;
    }

    device = cam->cfg->libcam_device;

    /* Enhanced camera selection:
     * - "auto" or "camera0": First Pi camera
     * - "camera1", "camera2", etc.: Pi camera by index
     * - "imx708", "imx477", etc.: Pi camera by sensor model
     */
    if (device == "auto" || device == "camera0") {
        camera = pi_cams[0];
        MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO
            , "Selected first Pi camera (auto/camera0)");
    } else if (device.length() > 6 && device.substr(0, 6) == "camera") {
        /* Camera by index: camera0, camera1, etc. */
        int idx = 0;
        try {
            idx = std::stoi(device.substr(6));
        } catch (...) {
            MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
                , "Invalid camera index in '%s'", device.c_str());
            return -1;
        }

        if (idx < 0 || idx >= (int)pi_cams.size()) {
            MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
                , "Camera index %d out of range (have %d Pi cameras)"
                , idx, (int)pi_cams.size());
            return -1;
        }
        camera = pi_cams[(size_t)idx];
        MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO
            , "Selected Pi camera by index: %d", idx);
    } else {
        /* Try to find camera by sensor model name */
        camera = find_camera_by_model(device);
        if (!camera) {
            MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
                , "Camera '%s' not found. Available cameras:", device.c_str());
            for (size_t i = 0; i < pi_cams.size(); i++) {
                MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
                    , "  camera%d: %s", (int)i, pi_cams[i]->id().c_str());
            }
            return -1;
        }
    }

    MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO
        , "Selected camera: %s", camera->id().c_str());

    camera->acquire();
    started_aqr = true;

    MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO, "Finished.");

    return 0;
}

void cls_libcam::config_control_item(std::string pname, std::string pvalue)
{
    if (pname == "AeMeteringMode") {
       controls.set(controls::AeMeteringMode, mtoi(pvalue));
    }
    if (pname == "AeConstraintMode") {
        controls.set(controls::AeConstraintMode, mtoi(pvalue));
    }
    if (pname == "AeExposureMode") {
        controls.set(controls::AeExposureMode, mtoi(pvalue));
    }
    if (pname == "ExposureValue") {
        controls.set(controls::ExposureValue, mtof(pvalue));
    }
    if (pname == "ExposureTime") {
        controls.set(controls::ExposureTime, mtoi(pvalue));
    }
    if (pname == "AnalogueGain") {
        controls.set(controls::AnalogueGain, mtof(pvalue));
    }
    if (pname == "Brightness") {
        controls.set(controls::Brightness, mtof(pvalue));
    }
    if (pname == "Contrast") {
        controls.set(controls::Contrast, mtof(pvalue));
    }
    if (pname == "Lux") {
        controls.set(controls::Lux, mtof(pvalue));
    }
    if (pname == "AwbEnable") {
        controls.set(controls::AwbEnable, mtob(pvalue));
    }
    if (pname == "AwbMode") {
        controls.set(controls::AwbMode, mtoi(pvalue));
    }
    if (pname == "AwbLocked") {
        controls.set(controls::AwbLocked, mtob(pvalue));
    }
    if (pname == "ColourGains") {
        float cg[2];
        cg[0] = mtof(mtok(pvalue,"|"));
        cg[1] = mtof(mtok(pvalue,"|"));
        controls.set(controls::ColourGains, cg);
    }
    if (pname == "ColourTemperature") {
        controls.set(controls::ColourTemperature, mtoi(pvalue));
    }
    if (pname == "Saturation") {
        controls.set(controls::Saturation, mtof(pvalue));
    }
    if (pname == "SensorBlackLevels") {
        int32_t sbl[4];
        sbl[0] = mtoi(mtok(pvalue,"|"));
        sbl[1] = mtoi(mtok(pvalue,"|"));
        sbl[2] = mtoi(mtok(pvalue,"|"));
        sbl[3] = mtoi(mtok(pvalue,"|"));
        controls.set(controls::SensorBlackLevels, sbl);
    }
    if (pname == "Sharpness") {
        controls.set(controls::Sharpness, mtof(pvalue));
    }
    if (pname == "FocusFoM") {
        controls.set(controls::FocusFoM, mtoi(pvalue));
    }
    if (pname == "ColourCorrectionMatrix") {
        float ccm[9];
        ccm[0] = mtof(mtok(pvalue,"|"));
        ccm[1] = mtof(mtok(pvalue,"|"));
        ccm[2] = mtof(mtok(pvalue,"|"));
        ccm[3] = mtof(mtok(pvalue,"|"));
        ccm[4] = mtof(mtok(pvalue,"|"));
        ccm[5] = mtof(mtok(pvalue,"|"));
        ccm[6] = mtof(mtok(pvalue,"|"));
        ccm[7] = mtof(mtok(pvalue,"|"));
        ccm[8] = mtof(mtok(pvalue,"|"));
        controls.set(controls::ColourCorrectionMatrix, ccm);
    }
    if (pname == "ScalerCrop") {
        Rectangle crop;
        crop.x = mtoi(mtok(pvalue,"|"));
        crop.y = mtoi(mtok(pvalue,"|"));
        crop.width =(uint)mtoi(mtok(pvalue,"|"));
        crop.height =(uint)mtoi(mtok(pvalue,"|"));
        controls.set(controls::ScalerCrop, crop);
    }
    if (pname == "DigitalGain") {
        controls.set(controls::DigitalGain, mtof(pvalue));
    }
    if (pname == "FrameDuration") {
        controls.set(controls::FrameDuration, mtoi(pvalue));
    }
    if (pname == "FrameDurationLimits") {
        int64_t fdl[2];
        fdl[0] = mtol(mtok(pvalue,"|"));
        fdl[1] = mtol(mtok(pvalue,"|"));
        controls.set(controls::FrameDurationLimits, fdl);
    }
    if (pname == "SensorTemperature") {
        controls.set(controls::SensorTemperature, mtof(pvalue));
    }
    if (pname == "SensorTimestamp") {
        controls.set(controls::SensorTimestamp, mtoi(pvalue));
    }
    if (pname == "AfMode") {
        controls.set(controls::AfMode, mtoi(pvalue));
    }
    if (pname == "AfRange") {
        controls.set(controls::AfRange, mtoi(pvalue));
    }
    if (pname == "AfSpeed") {
        controls.set(controls::AfSpeed, mtoi(pvalue));
    }
    if (pname == "AfMetering") {
        controls.set(controls::AfMetering, mtoi(pvalue));
    }
    if (pname == "AfWindows") {
        Rectangle afwin[1];
        afwin[0].x = mtoi(mtok(pvalue,"|"));
        afwin[0].y = mtoi(mtok(pvalue,"|"));
        afwin[0].width = (uint)mtoi(mtok(pvalue,"|"));
        afwin[0].height = (uint)mtoi(mtok(pvalue,"|"));
        controls.set(controls::AfWindows, afwin);
    }
    if (pname == "AfTrigger") {
        controls.set(controls::AfTrigger, mtoi(pvalue));
    }
    if (pname == "AfPause") {
        controls.set(controls::AfPause, mtoi(pvalue));
    }
    if (pname == "LensPosition") {
        controls.set(controls::LensPosition, mtof(pvalue));
    }
    if (pname == "AfState") {
        controls.set(controls::AfState, mtoi(pvalue));
    }
    if (pname == "AfPauseState") {
        controls.set(controls::AfPauseState, mtoi(pvalue));
    }

    /* DRAFT*/
    if (pname == "AePrecaptureTrigger") {
        controls.set(controls::draft::AePrecaptureTrigger, mtoi(pvalue));
    }
    if (pname == "NoiseReductionMode") {
        controls.set(controls::draft::NoiseReductionMode, mtoi(pvalue));
    }
    if (pname == "ColorCorrectionAberrationMode") {
        controls.set(controls::draft::ColorCorrectionAberrationMode, mtoi(pvalue));
    }
    if (pname == "AwbState") {
        controls.set(controls::draft::AwbState, mtoi(pvalue));
    }
    if (pname == "SensorRollingShutterSkew") {
        controls.set(controls::draft::SensorRollingShutterSkew, mtoi(pvalue));
    }
    if (pname == "LensShadingMapMode") {
        controls.set(controls::draft::LensShadingMapMode, mtoi(pvalue));
    }
    if (pname == "PipelineDepth") {
        controls.set(controls::draft::PipelineDepth, mtoi(pvalue));
    }
    if (pname == "MaxLatency") {
        controls.set(controls::draft::MaxLatency, mtoi(pvalue));
    }
    if (pname == "TestPatternMode") {
        controls.set(controls::draft::TestPatternMode, mtoi(pvalue));
    }

}

void cls_libcam::config_controls()
{
    int retcd, indx;

    for (indx=0;indx<params->params_cnt;indx++) {
        config_control_item(
            params->params_array[indx].param_name
            ,params->params_array[indx].param_value);
    }

    /* Apply initial brightness/contrast/ISO from config */
    controls.set(controls::Brightness, cam->cfg->parm_cam.libcam_brightness);
    controls.set(controls::Contrast, cam->cfg->parm_cam.libcam_contrast);
    controls.set(controls::AnalogueGain, iso_to_gain(cam->cfg->parm_cam.libcam_iso));

    retcd = config->validate();
    if (retcd == CameraConfiguration::Adjusted) {
        MOTION_LOG(INF, TYPE_VIDEO, NO_ERRNO
            , "Configuration controls adjusted.");
    } else if (retcd == CameraConfiguration::Valid) {
         MOTION_LOG(DBG, TYPE_VIDEO, NO_ERRNO
            , "Configuration controls valid");
    } else if (retcd == CameraConfiguration::Invalid) {
         MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
            , "Configuration controls error");
    }

}

void cls_libcam:: config_orientation()
{
    #if (LIBCAMVER >= 2000)
        int retcd, indx;
        std::string adjdesc;
        ctx_params_item *itm;

        for (indx=0;indx<params->params_cnt;indx++) {
            itm = &params->params_array[indx];
            if (itm->param_name == "orientation") {
                if (itm->param_value == "Rotate0") {
                    config->orientation = Orientation::Rotate0;
                } else if (itm->param_value == "Rotate0Mirror") {
                    config->orientation = Orientation::Rotate0Mirror;
                } else if (itm->param_value == "Rotate180") {
                    config->orientation = Orientation::Rotate180;
                } else if (itm->param_value == "Rotate180Mirror") {
                    config->orientation = Orientation::Rotate180Mirror;
                } else if (itm->param_value == "Rotate90") {
                    config->orientation = Orientation::Rotate90;
                } else if (itm->param_value == "Rotate90Mirror") {
                    config->orientation = Orientation::Rotate90Mirror;
                } else if (itm->param_value == "Rotate270") {
                    config->orientation = Orientation::Rotate270;
                } else if (itm->param_value == "Rotate270Mirror") {
                    config->orientation = Orientation::Rotate270Mirror;
                } else {
                    MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
                        , "Invalid Orientation option: %s."
                        , itm->param_value.c_str());
                }
            }
        }

        retcd = config->validate();
        if (retcd == CameraConfiguration::Adjusted) {
            if (config->orientation == Orientation::Rotate0) {
                adjdesc = "Rotate0";
            } else if (config->orientation == Orientation::Rotate0Mirror) {
                adjdesc = "Rotate0Mirror";
            } else if (config->orientation == Orientation::Rotate90) {
                adjdesc = "Rotate90";
            } else if (config->orientation == Orientation::Rotate90Mirror) {
            adjdesc = "Rotate90Mirror";
            } else if (config->orientation == Orientation::Rotate180) {
                adjdesc = "Rotate180";
            } else if (config->orientation == Orientation::Rotate180Mirror) {
                adjdesc = "Rotate180Mirror";
            } else if (config->orientation == Orientation::Rotate270) {
                adjdesc = "Rotate270";
            } else if (config->orientation == Orientation::Rotate270Mirror) {
                adjdesc = "Rotate270Mirror";
            } else {
                adjdesc = "unknown";
            }
            MOTION_LOG(INF, TYPE_VIDEO, NO_ERRNO
                , "Configuration orientation adjusted to %s."
                , adjdesc.c_str());
        } else if (retcd == CameraConfiguration::Valid) {
            MOTION_LOG(DBG, TYPE_VIDEO, NO_ERRNO
                , "Configuration orientation valid");
        } else if (retcd == CameraConfiguration::Invalid) {
            MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
                , "Configuration orientation error");
        }
    #else
        MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO, "Orientation Not available");
    #endif

}

int cls_libcam::start_config()
{
    int retcd;
    int buffer_count;

    MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO, "Starting.");

    /* Pi 5 requires VideoRecording role for proper PiSP pipeline initialization */
    config = camera->generateConfiguration({ StreamRole::VideoRecording });

    config->at(0).pixelFormat = PixelFormat::fromString("YUV420");

    config->at(0).size.width = (uint)cam->cfg->width;
    config->at(0).size.height = (uint)cam->cfg->height;

    /* Multi-buffer support for Pi 5: default 4 buffers to prevent pipeline stalls */
    buffer_count = cam->cfg->libcam_buffer_count;
    if (buffer_count < 2) {
        buffer_count = 2;
    } else if (buffer_count > 8) {
        buffer_count = 8;
    }
    config->at(0).bufferCount = (uint)buffer_count;
    MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO, "Buffer count: %d", buffer_count);

    config->at(0).stride = 0;

    retcd = config->validate();
    if (retcd == CameraConfiguration::Adjusted) {
        if (config->at(0).pixelFormat != PixelFormat::fromString("YUV420")) {
            MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO
                , "Pixel format was adjusted to %s."
                , config->at(0).pixelFormat.toString().c_str());
            return -1;
        } else {
            MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO
                , "Configuration adjusted.");
        }
    } else if (retcd == CameraConfiguration::Valid) {
         MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO
            , "Configuration is valid");
    } else if (retcd == CameraConfiguration::Invalid) {
         MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
            , "Error setting configuration");
        return -1;
    }

    if ((config->at(0).size.width != (uint)cam->cfg->width) ||
        (config->at(0).size.height != (uint)cam->cfg->height)) {
        MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO
            , "Image size adjusted from %d x %d to %d x %d"
            , cam->cfg->width, cam->cfg->height
            , config->at(0).size.width, config->at(0).size.height);
    }

    cam->imgs.width = (int)config->at(0).size.width;
    cam->imgs.height = (int)config->at(0).size.height;
    cam->imgs.size_norm = (cam->imgs.width * cam->imgs.height * 3) / 2;
    cam->imgs.motionsize = cam->imgs.width * cam->imgs.height;

    log_orientation();
    log_controls();
    log_draft();

    config_orientation();
    config_controls();

    camera->configure(config.get());

    MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO, "Finished.");

    return 0;
}

int cls_libcam::req_add(Request *request)
{
    int retcd;

    /* Apply pending brightness/contrast/ISO/AWB controls if changed */
    if (pending_ctrls.dirty.load()) {
        ControlList &req_controls = request->controls();
        req_controls.set(controls::Brightness, pending_ctrls.brightness);
        req_controls.set(controls::Contrast, pending_ctrls.contrast);
        req_controls.set(controls::AnalogueGain, iso_to_gain(pending_ctrls.iso));

        // Apply AWB controls
        req_controls.set(controls::AwbEnable, pending_ctrls.awb_enable);
        req_controls.set(controls::AwbMode, pending_ctrls.awb_mode);
        req_controls.set(controls::AwbLocked, pending_ctrls.awb_locked);

        // Apply manual colour temperature if set (non-zero)
        if (pending_ctrls.colour_temp > 0) {
            req_controls.set(controls::ColourTemperature, pending_ctrls.colour_temp);
        }

        // Apply manual colour gains if set (non-zero)
        if (pending_ctrls.colour_gain_r > 0.0f || pending_ctrls.colour_gain_b > 0.0f) {
            float cg[2] = {pending_ctrls.colour_gain_r, pending_ctrls.colour_gain_b};
            req_controls.set(controls::ColourGains, cg);
        }

        pending_ctrls.dirty.store(false);
        MOTION_LOG(DBG, TYPE_VIDEO, NO_ERRNO
            , "Applied controls to request: brightness=%.2f, contrast=%.2f, iso=%.0f (gain=%.2f), awb_enable=%s, awb_mode=%d"
            , pending_ctrls.brightness, pending_ctrls.contrast
            , pending_ctrls.iso, iso_to_gain(pending_ctrls.iso)
            , pending_ctrls.awb_enable ? "true" : "false", pending_ctrls.awb_mode);
    }

    retcd = camera->queueRequest(request);
    return retcd;
}

int cls_libcam::start_req()
{
    int retcd, bytes, indx, width;
    unsigned int buf_idx;

    MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO, "Starting.");

    camera->requestCompleted.connect(this, &cls_libcam::req_complete);
    frmbuf = std::make_unique<FrameBufferAllocator>(camera);

    retcd = frmbuf->allocate(config->at(0).stream());
    if (retcd < 0) {
        MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
            , "Buffer allocation error.");
        return -1;
    }

    Stream *stream = config->at(0).stream();
    const std::vector<std::unique_ptr<FrameBuffer>> &buffers =
        frmbuf->buffers(stream);

    MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO
        , "Allocated %d buffers for stream", (int)buffers.size());

    /* Create a request for each buffer in the pool */
    for (buf_idx = 0; buf_idx < buffers.size(); buf_idx++) {
        std::unique_ptr<Request> request = camera->createRequest();
        if (!request) {
            MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
                , "Create request error for buffer %d.", buf_idx);
            return -1;
        }

        retcd = request->addBuffer(stream, buffers[buf_idx].get());
        if (retcd < 0) {
            MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
                , "Add buffer %d for request error.", buf_idx);
            return -1;
        }

        requests.push_back(std::move(request));
    }

    started_req = true;

    /* Calculate buffer size from first buffer (all buffers same size) */
    const std::unique_ptr<FrameBuffer> &buffer = buffers[0];
    const FrameBuffer::Plane &plane0 = buffer->planes()[0];

    bytes = 0;
    for (indx = 0; indx < (int)buffer->planes().size(); indx++) {
        bytes += buffer->planes()[(uint)indx].length;
        MOTION_LOG(DBG, TYPE_VIDEO, NO_ERRNO, "Plane %d of %d length %d"
            , indx, (int)buffer->planes().size()
            , buffer->planes()[(uint)indx].length);
    }

    /* Adjust image dimensions if buffer size doesn't match expected */
    if (bytes > cam->imgs.size_norm) {
        width = ((int)buffer->planes()[0].length / cam->imgs.height);
        if (((int)buffer->planes()[0].length != (width * cam->imgs.height)) ||
            (bytes > ((width * cam->imgs.height * 3) / 2))) {
            MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
                , "Error setting image size.  Plane 0 length %d, total bytes %d"
                , buffer->planes()[0].length, bytes);
        }
        MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO
            , "Image size adjusted from %d x %d to %d x %d"
            , cam->imgs.width, cam->imgs.height
            , width, cam->imgs.height);
        cam->imgs.width = width;
        cam->imgs.size_norm = (cam->imgs.width * cam->imgs.height * 3) / 2;
        cam->imgs.motionsize = cam->imgs.width * cam->imgs.height;
    }

    /* Map memory for legacy single-buffer access (backwards compatibility) */
    membuf.buf = (uint8_t *)mmap(NULL, (uint)bytes, PROT_READ
        , MAP_SHARED, plane0.fd.get(), 0);
    membuf.bufsz = bytes;

    /* Map memory for each buffer in the pool */
    membuf_pool.clear();
    for (buf_idx = 0; buf_idx < buffers.size(); buf_idx++) {
        ctx_imgmap map;
        const FrameBuffer::Plane &p0 = buffers[buf_idx]->planes()[0];

        int buf_bytes = 0;
        for (const auto &plane : buffers[buf_idx]->planes()) {
            buf_bytes += plane.length;
        }

        map.buf = (uint8_t *)mmap(NULL, (uint)buf_bytes, PROT_READ
            , MAP_SHARED, p0.fd.get(), 0);
        map.bufsz = buf_bytes;

        if (map.buf == MAP_FAILED) {
            MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
                , "mmap failed for buffer %d", buf_idx);
            return -1;
        }

        membuf_pool.push_back(map);
        MOTION_LOG(DBG, TYPE_VIDEO, NO_ERRNO
            , "Mapped buffer %d: %d bytes", buf_idx, buf_bytes);
    }

    MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO
        , "Finished. Created %d requests with %d mapped buffers."
        , (int)requests.size(), (int)membuf_pool.size());

    return 0;
}

int cls_libcam::start_capture()
{
    int retcd;

    MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO, "Starting.");

    retcd = camera->start(&this->controls);
    if (retcd) {
        MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
            , "Failed to start capture.");
        return -1;
    }
    controls.clear();

    for (std::unique_ptr<Request> &request : requests) {
        retcd = req_add(request.get());
        if (retcd < 0) {
            MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO
                , "Failed to queue request.");
            if (started_cam) {
                camera->stop();
            }
            return -1;
        }
    }
    MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO, "Finished.");

    return 0;
}

void cls_libcam::req_complete(Request *request)
{
    if (request->status() == Request::RequestCancelled) {
        return;
    }

    /* Find which buffer index this request corresponds to */
    Stream *stream = config->at(0).stream();
    FrameBuffer *buffer = request->findBuffer(stream);

    const std::vector<std::unique_ptr<FrameBuffer>> &buffers =
        frmbuf->buffers(stream);

    int buf_idx = -1;
    for (size_t i = 0; i < buffers.size(); i++) {
        if (buffers[i].get() == buffer) {
            buf_idx = (int)i;
            break;
        }
    }

    if (buf_idx >= 0 && buf_idx < (int)membuf_pool.size()) {
        ctx_reqinfo req_info;
        req_info.request = request;
        req_info.buffer_idx = buf_idx;
        req_queue.push(req_info);
    } else {
        MOTION_LOG(WRN, TYPE_VIDEO, NO_ERRNO
            , "Request completed with unknown buffer index");
        /* Fallback: use buffer 0 if available */
        if (!membuf_pool.empty()) {
            ctx_reqinfo req_info;
            req_info.request = request;
            req_info.buffer_idx = 0;
            req_queue.push(req_info);
        }
    }
}

int cls_libcam::libcam_start()
{
    started_cam = false;
    started_mgr = false;
    started_aqr = false;
    started_req = false;

    start_params();

    if (start_mgr() != 0) {
        return -1;
    }
    if (start_config() != 0) {
        return -1;
    }
    if (start_req() != 0) {
        return -1;
    }
    if (start_capture() != 0) {
        return -1;
    }

    cam->watchdog = cam->cfg->watchdog_tmo;
    SLEEP(1,0);

    started_cam = true;

    MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO, "Camera started");

    return 0;
}

void cls_libcam::libcam_stop()
{
    mydelete(params);

    if (started_aqr) {
        camera->stop();
    }

    if (started_req) {
        camera->requestCompleted.disconnect(this, &cls_libcam::req_complete);
        while (req_queue.empty() == false) {
            req_queue.pop();
        }
        requests.clear();

        frmbuf->free(config->at(0).stream());
        frmbuf.reset();
    }

    controls.clear();

    if (started_aqr){
        camera->release();
        camera.reset();
    }
    if (started_mgr) {
        cam_mgr->stop();
        cam_mgr.reset();
    }
    cam->device_status = STATUS_CLOSED;
    MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO, "Camera stopped.");
}

#endif

void cls_libcam::set_brightness(float value)
{
    #ifdef HAVE_LIBCAM
        pending_ctrls.brightness = value;
        pending_ctrls.dirty.store(true);
        MOTION_LOG(DBG, TYPE_VIDEO, NO_ERRNO
            , "Hot-reload: brightness set to %.2f", value);
    #else
        (void)value;
    #endif
}

void cls_libcam::set_contrast(float value)
{
    #ifdef HAVE_LIBCAM
        pending_ctrls.contrast = value;
        pending_ctrls.dirty.store(true);
        MOTION_LOG(DBG, TYPE_VIDEO, NO_ERRNO
            , "Hot-reload: contrast set to %.2f", value);
    #else
        (void)value;
    #endif
}

void cls_libcam::set_iso(float value)
{
    #ifdef HAVE_LIBCAM
        pending_ctrls.iso = value;
        pending_ctrls.dirty.store(true);
        MOTION_LOG(DBG, TYPE_VIDEO, NO_ERRNO
            , "Hot-reload: ISO set to %.0f (gain=%.2f)", value, iso_to_gain(value));
    #else
        (void)value;
    #endif
}

void cls_libcam::set_awb_enable(bool value)
{
    #ifdef HAVE_LIBCAM
        pending_ctrls.awb_enable = value;
        pending_ctrls.dirty.store(true);
        MOTION_LOG(DBG, TYPE_VIDEO, NO_ERRNO
            , "Hot-reload: AWB enable set to %s", value ? "true" : "false");
    #else
        (void)value;
    #endif
}

void cls_libcam::set_awb_mode(int value)
{
    #ifdef HAVE_LIBCAM
        pending_ctrls.awb_mode = value;
        pending_ctrls.dirty.store(true);
        MOTION_LOG(DBG, TYPE_VIDEO, NO_ERRNO
            , "Hot-reload: AWB mode set to %d", value);
    #else
        (void)value;
    #endif
}

void cls_libcam::set_awb_locked(bool value)
{
    #ifdef HAVE_LIBCAM
        pending_ctrls.awb_locked = value;
        pending_ctrls.dirty.store(true);
        MOTION_LOG(DBG, TYPE_VIDEO, NO_ERRNO
            , "Hot-reload: AWB locked set to %s", value ? "true" : "false");
    #else
        (void)value;
    #endif
}

void cls_libcam::set_colour_temp(int value)
{
    #ifdef HAVE_LIBCAM
        pending_ctrls.colour_temp = value;
        pending_ctrls.dirty.store(true);
        MOTION_LOG(DBG, TYPE_VIDEO, NO_ERRNO
            , "Hot-reload: Colour temperature set to %dK", value);
    #else
        (void)value;
    #endif
}

void cls_libcam::set_colour_gains(float red, float blue)
{
    #ifdef HAVE_LIBCAM
        pending_ctrls.colour_gain_r = red;
        pending_ctrls.colour_gain_b = blue;
        pending_ctrls.dirty.store(true);
        MOTION_LOG(DBG, TYPE_VIDEO, NO_ERRNO
            , "Hot-reload: Colour gains set to R=%.2f, B=%.2f", red, blue);
    #else
        (void)red; (void)blue;
    #endif
}

void cls_libcam::apply_pending_controls()
{
    #ifdef HAVE_LIBCAM
        /* Note: libcamera 0.5.2 doesn't have Camera::setControls().
         * Controls are applied via requests. For now, we'll apply them
         * on the next queued request in req_add(). The dirty flag signals
         * that controls should be updated on next request.
         */
        if (pending_ctrls.dirty.load()) {
            MOTION_LOG(INF, TYPE_VIDEO, NO_ERRNO
                , "Brightness/Contrast/ISO update pending: brightness=%.2f, contrast=%.2f, iso=%.0f"
                , pending_ctrls.brightness, pending_ctrls.contrast, pending_ctrls.iso);
        }
    #endif
}

void cls_libcam::noimage()
{
    #ifdef HAVE_LIBCAM
        int slp_dur;

        if (reconnect_count < 100) {
            reconnect_count++;
        } else {
            if (reconnect_count >= 500) {
                MOTION_LOG(NTC, TYPE_NETCAM, NO_ERRNO,_("Camera did not reconnect."));
                MOTION_LOG(NTC, TYPE_NETCAM, NO_ERRNO,_("Checking for camera every 2 hours."));
                slp_dur = 7200;
            } else if (reconnect_count >= 200) {
                MOTION_LOG(NTC, TYPE_NETCAM, NO_ERRNO,_("Camera did not reconnect."));
                MOTION_LOG(NTC, TYPE_NETCAM, NO_ERRNO,_("Checking for camera every 10 minutes."));
                reconnect_count++;
                slp_dur = 600;
            } else {
                MOTION_LOG(NTC, TYPE_NETCAM, NO_ERRNO,_("Camera did not reconnect."));
                MOTION_LOG(NTC, TYPE_NETCAM, NO_ERRNO,_("Checking for camera every 30 seconds."));
                reconnect_count++;
                slp_dur = 30;
            }
            cam->watchdog = slp_dur + (cam->cfg->watchdog_tmo * 3);
            SLEEP(slp_dur,0);
            libcam_stop();
            if (libcam_start() < 0) {
                MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO,_("libcam failed to open"));
                libcam_stop();
            } else {
                cam->device_status = STATUS_OPENED;
            }
        }
    #endif
}

int cls_libcam::next(ctx_image_data *img_data)
{
    #ifdef HAVE_LIBCAM
        int indx;

        if (started_cam == false) {
            return CAPTURE_FAILURE;
        }

        cam->watchdog = cam->cfg->watchdog_tmo;

        /* Allow time for request to finish */
        indx = 0;
        while (req_queue.empty() && indx < 50) {
            SLEEP(0, 2000);
            indx++;
        }

        cam->watchdog = cam->cfg->watchdog_tmo;

        if (!req_queue.empty()) {
            /* Get request info including buffer index */
            ctx_reqinfo req_info = req_queue.front();
            req_queue.pop();

            Request *request = req_info.request;
            int buf_idx = req_info.buffer_idx;

            /* Copy frame data from the correct buffer in the pool */
            if (buf_idx >= 0 && buf_idx < (int)membuf_pool.size()) {
                memcpy(img_data->image_norm,
                       membuf_pool[(size_t)buf_idx].buf,
                       (uint)membuf_pool[(size_t)buf_idx].bufsz);
            } else {
                /* Fallback to legacy single buffer for compatibility */
                memcpy(img_data->image_norm, membuf.buf, (uint)membuf.bufsz);
            }

            /* Requeue request for next frame */
            request->reuse(Request::ReuseBuffers);
            req_add(request);

            cam->rotate->process(img_data);
            reconnect_count = 0;

            /* Apply any pending hot-reload control changes */
            apply_pending_controls();

            return CAPTURE_SUCCESS;

        } else {
            return CAPTURE_FAILURE;
        }
    #else
        (void)img_data;
        return CAPTURE_FAILURE;
    #endif
}

cls_libcam::cls_libcam(cls_camera *p_cam)
{
    cam = p_cam;
    #ifdef HAVE_LIBCAM
        MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO,_("Opening libcam"));
        params = nullptr;
        reconnect_count = 0;
        /* Initialize pending controls with config values */
        pending_ctrls.brightness = cam->cfg->parm_cam.libcam_brightness;
        pending_ctrls.contrast = cam->cfg->parm_cam.libcam_contrast;
        pending_ctrls.iso = cam->cfg->parm_cam.libcam_iso;
        pending_ctrls.awb_enable = cam->cfg->parm_cam.libcam_awb_enable;
        pending_ctrls.awb_mode = cam->cfg->parm_cam.libcam_awb_mode;
        pending_ctrls.awb_locked = cam->cfg->parm_cam.libcam_awb_locked;
        pending_ctrls.colour_temp = cam->cfg->parm_cam.libcam_colour_temp;
        pending_ctrls.colour_gain_r = cam->cfg->parm_cam.libcam_colour_gain_r;
        pending_ctrls.colour_gain_b = cam->cfg->parm_cam.libcam_colour_gain_b;
        pending_ctrls.dirty.store(false);
        cam->watchdog = cam->cfg->watchdog_tmo * 3; /* 3 is arbitrary multiplier to give startup more time*/
        if (libcam_start() < 0) {
            MOTION_LOG(ERR, TYPE_VIDEO, NO_ERRNO,_("libcam failed to open"));
            libcam_stop();
        } else {
            cam->device_status = STATUS_OPENED;
        }
    #else
        MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO,_("libcam not available"));
        cam->device_status = STATUS_CLOSED;
    #endif
}

cls_libcam::~cls_libcam()
{
    #ifdef HAVE_LIBCAM
        libcam_stop();
    #endif
    cam->device_status = STATUS_CLOSED;
}

