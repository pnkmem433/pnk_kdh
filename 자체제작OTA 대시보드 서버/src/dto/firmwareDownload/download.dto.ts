import { ApiProperty } from '@nestjs/swagger';
import { IsIn, IsInt, IsNotEmpty, IsString } from 'class-validator';

export class firmwareDownloadDto {
  @ApiProperty({
    description: '프로젝트 ID',
    example: 10,
    type: 'number',
  })
  @IsInt()
  @IsNotEmpty()
  projectId: number;

  @ApiProperty({
    description: '칩 종류',
    example: 'esp8685',
    type: 'string',
    enum: ['esp02s', 'esp8685'],
  })
  @IsString()
  @IsNotEmpty()
  @IsIn(['esp02s', 'esp8685'])
  chipType: string;

  @ApiProperty({
    description: '현재 펌웨어 계열',
    example: 'custom',
    type: 'string',
    enum: ['custom', 'tasmota'],
  })
  @IsString()
  @IsNotEmpty()
  @IsIn(['custom', 'tasmota'])
  currentFirmwareFamily: string;

  @ApiProperty({
    description: '현재 버전 번호',
    example: 23,
    type: 'number',
  })
  @IsInt()
  @IsNotEmpty()
  currentVersion: number;
}
