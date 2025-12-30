import { FormSection, FormInput, FormToggle } from '@/components/form';

export interface ScheduleSettingsProps {
  config: Record<string, { value: string | number | boolean }>;
  onChange: (param: string, value: string | number | boolean) => void;
  getError?: (param: string) => string | undefined;
}

export function ScheduleSettings({ config, onChange, getError }: ScheduleSettingsProps) {
  const getValue = (param: string, defaultValue: string | number | boolean = '') => {
    return config[param]?.value ?? defaultValue;
  };

  const scheduleParams = String(getValue('schedule_params', ''));
  const isScheduleEnabled = scheduleParams.trim() !== '';

  const handleEnableToggle = (enabled: boolean) => {
    if (!enabled) {
      // Disable schedule by clearing the parameter
      onChange('schedule_params', '');
    } else {
      // Enable with a default schedule (Monday-Friday 00:00-23:59)
      onChange('schedule_params', '00:00-23:59|00:00-23:59|00:00-23:59|00:00-23:59|00:00-23:59||');
    }
  };

  return (
    <FormSection
      title="Motion Detection Schedule"
      description="Configure when motion detection is active"
      collapsible
      defaultOpen={false}
    >
      <div className="space-y-4">
        <FormToggle
          label="Enable Schedule"
          value={isScheduleEnabled}
          onChange={handleEnableToggle}
          helpText="When enabled, motion detection only runs during specified hours"
        />

        {isScheduleEnabled && (
          <>
            <FormInput
              label="Schedule Parameters"
              value={scheduleParams}
              onChange={(val) => onChange('schedule_params', val)}
              helpText="Pipe-separated time ranges for each day: Mon|Tue|Wed|Thu|Fri|Sat|Sun"
              error={getError?.('schedule_params')}
            />

            <div className="border-t border-surface-elevated pt-4">
              <h4 className="font-medium mb-2 text-sm">Schedule Format</h4>
              <div className="text-xs text-gray-400 space-y-2 bg-surface-elevated p-3 rounded">
                <p><strong>Format:</strong> Seven day periods separated by pipes (|), one for each day starting with Monday</p>
                <p><strong>Time Range:</strong> HH:MM-HH:MM (24-hour format)</p>
                <p><strong>Empty Day:</strong> Leave empty (just |) to disable motion detection for that day</p>

                <div className="mt-3 space-y-1">
                  <p className="font-medium">Examples:</p>
                  <p><code>00:00-23:59|00:00-23:59|00:00-23:59|00:00-23:59|00:00-23:59||</code></p>
                  <p className="text-xs">→ Weekdays 24/7, weekends off</p>

                  <p className="mt-2"><code>08:00-18:00|08:00-18:00|08:00-18:00|08:00-18:00|08:00-18:00||</code></p>
                  <p className="text-xs">→ Weekdays 8am-6pm only</p>

                  <p className="mt-2"><code>||||||00:00-23:59</code></p>
                  <p className="text-xs">→ Sundays only</p>

                  <p className="mt-2"><code>18:00-08:00|18:00-08:00|18:00-08:00|18:00-08:00|18:00-08:00|18:00-08:00|18:00-08:00</code></p>
                  <p className="text-xs">→ Every night 6pm-8am (overnight schedules supported)</p>
                </div>

                <p className="mt-3 text-yellow-200">
                  <strong>Note:</strong> Schedule format is Motion's native pipe-separated format.
                  A visual day/time picker will be available in a future update.
                </p>
              </div>
            </div>

            <div className="text-xs text-blue-200 bg-blue-600/10 border border-blue-600/30 p-3 rounded">
              <strong>💡 Tip:</strong> Test your schedule by checking Motion logs to verify detection
              starts and stops at the expected times.
            </div>
          </>
        )}
      </div>
    </FormSection>
  );
}
