import { ApiProperty } from '@nestjs/swagger';
import { IsString, MinLength } from 'class-validator';

export class UserUpdatePasswordDto {
  @ApiProperty({
    description: '현제 비밀번호',
    example: 'userPassword123',
    type: 'string',
  })
  @IsString()
  current_password: string;

  @ApiProperty({
    description: '바꿀 비밀번호',
    example: 'userPassword123',
    type: 'string',
  })
  @IsString()
  @MinLength(8, { message: '비밀번호는 8글자 이상이어야 합니다.' })
  new_password: string;
}
