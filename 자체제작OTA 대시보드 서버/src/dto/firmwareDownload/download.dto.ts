import { ApiProperty } from '@nestjs/swagger';
import { IsIn, IsInt, IsNotEmpty, IsOptional, IsString } from 'class-validator';

export class firmwareDownloadDto {
  @ApiProperty({
    description: 'Project ID',
    example: 10,
    type: 'number',
  })
  @IsInt()
  @IsNotEmpty()
  projectId: number;

  @ApiProperty({
    description: 'Chip type',
    example: 'esp8685',
    type: 'string',
    enum: ['esp02s', 'esp8685'],
  })
  @IsString()
  @IsNotEmpty()
  @IsIn(['esp02s', 'esp8685'])
  chipType: string;

  @ApiProperty({
    description: 'Current firmware family',
    example: 'custom',
    type: 'string',
    enum: ['custom', 'tasmota'],
  })
  @IsString()
  @IsNotEmpty()
  @IsIn(['custom', 'tasmota'])
  currentFirmwareFamily: string;

  @ApiProperty({
    description: 'Current version number',
    example: 23,
    type: 'number',
  })
  @IsInt()
  @IsNotEmpty()
  currentVersion: number;

  @ApiProperty({
    description: 'Device MAC address without separators',
    example: 'A1B2C3D4E5F6',
    type: 'string',
    required: false,
  })
  @IsString()
  @IsOptional()
  macAddress?: string;

  @ApiProperty({
    description: 'Optional boot source hint from the custom firmware',
    example: 'external_install',
    type: 'string',
    required: false,
    enum: ['stable', 'custom_ota', 'external_install'],
  })
  @IsString()
  @IsOptional()
  @IsIn(['stable', 'custom_ota', 'external_install'])
  bootSourceHint?: string;
}
