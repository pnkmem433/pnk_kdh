import { ApiProperty } from '@nestjs/swagger';
import { IsString, MinLength } from 'class-validator';

export class ProjectCreateDto {
  @ApiProperty({
    description: '프로젝트 이름',
    example: '새 프로젝트 1',
    type: 'string',
  })
  @IsString()
  newName: string;

}
