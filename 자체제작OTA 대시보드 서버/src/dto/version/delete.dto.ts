import { ApiProperty } from '@nestjs/swagger';
import { IsIn, IsInt, IsNotEmpty, IsString } from 'class-validator';

export class VersionDeleteDto {
  @ApiProperty({
    description: 'Project ID',
    type: 'integer',
    example: 10,
  })
  @IsInt()
  @IsNotEmpty()
  projectId: number;

  @ApiProperty({
    description: 'Version number to hard delete',
    type: 'integer',
    example: 95,
  })
  @IsInt()
  @IsNotEmpty()
  versionNumber: number;

  @ApiProperty({
    description: 'Chip type',
    type: 'string',
    enum: ['esp02s', 'esp8685'],
    example: 'esp02s',
  })
  @IsString()
  @IsNotEmpty()
  @IsIn(['esp02s', 'esp8685'])
  chipType: string;

  @ApiProperty({
    description: 'Firmware family',
    type: 'string',
    enum: ['custom', 'tasmota'],
    example: 'custom',
  })
  @IsString()
  @IsNotEmpty()
  @IsIn(['custom', 'tasmota'])
  firmwareFamily: string;
}
