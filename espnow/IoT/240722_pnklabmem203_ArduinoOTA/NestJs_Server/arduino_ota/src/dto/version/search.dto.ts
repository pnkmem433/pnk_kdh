import { ApiProperty } from '@nestjs/swagger';
import { IsString, MinLength } from 'class-validator';

export class VersionSearchDto {
  @ApiProperty({
    description: '버전을 조회할 프로젝트 id',
    example: 1,
    type: 'number',
  })
  @IsString()
  projectId: number;
}
