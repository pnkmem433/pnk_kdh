import { ApiProperty } from '@nestjs/swagger';
import { IsString, MinLength } from 'class-validator';

export class UserUpdateNameDto {
  @ApiProperty({
    description: '변경할 사용자 이름',
    example: '홍길동12',
    type: 'string',
  })
  @IsString()
  newName: string;

}
