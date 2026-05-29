import { ApiProperty } from '@nestjs/swagger';
import { IsString, MinLength } from 'class-validator';

export class UserLoginDto {
  @ApiProperty({
    description: '아이디',
    example: 'userId123',
    type: 'string',
  })
  @IsString()
  id: string;

  @ApiProperty({
    description: '비밀번호',
    example: 'userPassword123',
    type: 'string',
  })
  @IsString()
  password: string;
}
