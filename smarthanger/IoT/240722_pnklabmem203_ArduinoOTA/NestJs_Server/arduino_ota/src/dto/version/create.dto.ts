import { ApiProperty } from '@nestjs/swagger';
import { IsInt, IsString, IsNotEmpty, IsOptional } from 'class-validator';

export class VersionCreateDto {
  @ApiProperty({
    description: '프로젝트 ID',
    type: 'integer',
    example: 1,
  })
  @IsInt()
  @IsNotEmpty()
  projectId: number;

  @ApiProperty({
    description: '버전 번호',
    type: 'integer',
    example: 1,
  })
  @IsInt()
  @IsNotEmpty()
  versionNumber: number;

  @ApiProperty({
    description: '버전 이름',
    type: 'string',
    example: 'v1.0.0',
  })
  @IsString()
  @IsNotEmpty()
  versionName: string;

  @ApiProperty({
    description: 'Bin 파일 (바이너리 데이터)',
    type: 'string',
    format: 'binary',
  })
  binFile?: string;
}
