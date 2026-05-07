import { ApiProperty } from '@nestjs/swagger';
import { IsInt, IsString, IsNotEmpty, IsIn } from 'class-validator';

export class VersionCreateDto {
  @ApiProperty({
    description: '프로젝트 ID',
    type: 'integer',
    example: 10,
  })
  @IsInt()
  @IsNotEmpty()
  projectId: number;

  @ApiProperty({
    description: '버전 번호',
    type: 'integer',
    example: 23,
  })
  @IsInt()
  @IsNotEmpty()
  versionNumber: number;

  @ApiProperty({
    description: '버전 이름',
    type: 'string',
    example: 'v23_custom_esp8685',
  })
  @IsString()
  @IsNotEmpty()
  versionName: string;

  @ApiProperty({
    description: '칩 종류',
    type: 'string',
    enum: ['esp02s', 'esp8685'],
    example: 'esp8685',
  })
  @IsString()
  @IsNotEmpty()
  @IsIn(['esp02s', 'esp8685'])
  chipType: string;

  @ApiProperty({
    description: '업데이트할 펌웨어 계열',
    type: 'string',
    enum: ['custom', 'tasmota'],
    example: 'custom',
  })
  @IsString()
  @IsNotEmpty()
  @IsIn(['custom', 'tasmota'])
  firmwareFamily: string;

  @ApiProperty({
    description: 'Bin 파일',
    type: 'string',
    format: 'binary',
  })
  binFile?: string;
}
