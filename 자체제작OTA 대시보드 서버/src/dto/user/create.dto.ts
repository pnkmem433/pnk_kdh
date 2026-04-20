import { ApiProperty } from '@nestjs/swagger';
import { IsString, MinLength } from 'class-validator';

export class UserCreateDto {
  @ApiProperty({
    description: '아이디',
    example: 'userId123',
    type: 'string',
  })
  @IsString()
  @MinLength(3, { message: '아이디는 3글자 이상이어야 합니다.' })
  id: string;

  @ApiProperty({
    description: '사용자 이름',
    example: '홍길동',
    type: 'string',
  })
  @IsString()
  name: string;

  @ApiProperty({
    description: '비밀번호',
    example: 'userPassword123',
    type: 'string',
  })
  @IsString()
  @MinLength(8, { message: '비밀번호는 8글자 이상이어야 합니다.' })
  password: string;
}
