ALTER TABLE project_version
  ADD COLUMN IF NOT EXISTS "chipType" character varying(32) NOT NULL DEFAULT 'esp8685',
  ADD COLUMN IF NOT EXISTS "firmwareFamily" character varying(32) NOT NULL DEFAULT 'custom';
