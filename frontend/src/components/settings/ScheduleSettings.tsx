import { FormSection, FormToggle } from '@/components/form';
import { SchedulePicker } from '@/components/schedule';

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
      // Enable with a default schedule (Mon-Fri business hours paused)
      onChange('schedule_params', 'default=true action=pause mon-fri=0900-1700');
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
          helpText="When enabled, motion detection follows the schedule below"
        />

        {isScheduleEnabled && (
          <SchedulePicker
            value={scheduleParams}
            onChange={(val) => onChange('schedule_params', val)}
            error={getError?.('schedule_params')}
          />
        )}
      </div>
    </FormSection>
  );
}
