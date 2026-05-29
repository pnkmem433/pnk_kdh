import { ApiProperty } from '@nestjs/swagger';
import { IsString, IsNumber } from 'class-validator';

export class firmwareDownloadDto {
  @ApiProperty({
    description: '프로젝트 고유 uuid',
    example: '9136ee8a-f68f-4ffa-9d1e-70f5efa8f6d6',
    type: 'string',
  })
  @IsString()
  ProjectId: string;

  @ApiProperty({
    description: '현재 버전 번호',
    example: 1,
    type: 'number',
  })
  @IsNumber()
  currentVersion: number;
}
