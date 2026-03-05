/**
 * Profile conflict resolution — mirrors mutual-exclusivity rules from libcam.cpp:1485-1560.
 *
 * When a profile is applied it may contain contradictory libcam params (e.g. awb_enable=true
 * alongside colour_temp=4000). These functions resolve the conflicts before the params are
 * handed to the Settings "changes" state or the batch-config API so the backend never receives
 * contradictory values.
 */

export type ProfileParamValue = string | number | boolean

/**
 * Convert raw string profile params to typed JS values.
 *
 * Profiles stored via the API use string values for every parameter.
 * This function converts them to the types expected by the Settings
 * "changes" state (string | number | boolean).
 */
export function convertProfileParams(
  params: Record<string, string>
): Record<string, ProfileParamValue> {
  const result: Record<string, ProfileParamValue> = {}

  for (const [key, value] of Object.entries(params)) {
    if (value === 'true' || value === 'on') {
      result[key] = true
    } else if (value === 'false' || value === 'off') {
      result[key] = false
    } else if (value !== '' && !isNaN(Number(value))) {
      result[key] = Number(value)
    } else {
      result[key] = value
    }
  }

  return result
}

function isTruthy(v: ProfileParamValue | undefined): boolean {
  if (v === undefined) return false
  if (typeof v === 'boolean') return v
  if (typeof v === 'number') return v !== 0
  return v === 'true' || v === 'on' || v === '1'
}

/**
 * Resolve mutual-exclusivity conflicts in profile params.
 *
 * Rules (mirroring libcam.cpp set_* functions):
 *  1. awb_enable=true  → discard colour_temp, colour_gain_r, colour_gain_b
 *  2. awb_mode != 7    → discard colour_temp, colour_gain_r, colour_gain_b
 *  3. colour_temp > 0 AND (gain_r > 0 OR gain_b > 0) → keep colour_temp, discard gains
 *  4. af_mode != 0     → discard lens_position
 *  5. awb_enable=false → discard awb_locked
 *
 * Input is expected to have already been run through convertProfileParams().
 */
export function resolveProfileConflicts(
  params: Record<string, ProfileParamValue>
): Record<string, ProfileParamValue> {
  const p = { ...params }

  const awbEnable = p['libcam_awb_enable']
  const awbMode   = p['libcam_awb_mode']
  const colourTemp = p['libcam_colour_temp']
  const gainR     = p['libcam_colour_gain_r']
  const gainB     = p['libcam_colour_gain_b']
  const afMode    = p['libcam_af_mode']

  // Rule 1: AWB enabled → manual colour controls are irrelevant
  if (isTruthy(awbEnable)) {
    delete p['libcam_colour_temp']
    delete p['libcam_colour_gain_r']
    delete p['libcam_colour_gain_b']
  }

  // Rule 2: AWB preset mode (not Custom=7) → manual colour controls are irrelevant
  if (awbMode !== undefined && Number(awbMode) !== 7) {
    delete p['libcam_colour_temp']
    delete p['libcam_colour_gain_r']
    delete p['libcam_colour_gain_b']
  }

  // Rule 3: colour_temp > 0 wins over gains (set_colour_temp precedence)
  // Only applies when AWB is off and mode is Custom (7) — i.e. both rules above didn't fire.
  if (
    colourTemp !== undefined &&
    Number(colourTemp) > 0 &&
    (Number(gainR) > 0 || Number(gainB) > 0) &&
    'libcam_colour_temp' in p  // wasn't already removed by rules 1/2
  ) {
    delete p['libcam_colour_gain_r']
    delete p['libcam_colour_gain_b']
  }

  // Rule 4: autofocus active → manual lens position is irrelevant
  if (afMode !== undefined && Number(afMode) !== 0) {
    delete p['libcam_lens_position']
  }

  // Rule 5: AWB disabled → AWB lock is irrelevant
  if (awbEnable !== undefined && !isTruthy(awbEnable)) {
    delete p['libcam_awb_locked']
  }

  return p
}
