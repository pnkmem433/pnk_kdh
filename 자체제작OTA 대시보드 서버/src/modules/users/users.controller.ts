import { Controller, Post, Body, UnauthorizedException, Patch, UseGuards, Req } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiResponse, ApiBody, ApiOkResponse, ApiBadRequestResponse, ApiBearerAuth } from '@nestjs/swagger';
import { AuthGuard } from '@nestjs/passport';

import { UsersService } from './users.service';

import { UserCreateDto } from '../../dto/user/create.dto';
import { UserUpdateNameDto } from '../../dto/user/update-name.dto';
import { UserUpdatePasswordDto } from '../../dto/user/update-password';

@ApiTags('users')
@Controller('users')
export class UsersController {
  constructor(private readonly usersService: UsersService) { }

  @Post('register')
  @ApiOperation({ summary: '새 사용자 등록', description: '새로운 사용자를 등록합니다. 사용자 ID, 이름, 비밀번호를 포함한 요청이 필요합니다.' })
  @ApiBody({ description: '사용자 생성 바디', type: UserCreateDto })
  @ApiOkResponse({ description: '사용자가 성공적으로 등록되었습니다', example: { status: 200, message: "사용자가 성공적으로 등록되었습니다." } })
  @ApiBadRequestResponse({ description: '잘못된 요청입니다. 입력값이 유효하지 않거나 형식이 맞지 않는 경우 발생합니다.' })
  async register(@Body() userRegisterDto: UserCreateDto): Promise<{ message: string }> {
    return this.usersService.create(userRegisterDto);
  }

  @Patch('update-name')
  @UseGuards(AuthGuard('jwt'))
  @ApiBearerAuth()
  @ApiOperation({ summary: '사용자 이름 변경', description: '로그인한 사용자의 이름을 변경합니다.' })
  @ApiBody({ description: '사용자 이름 변경 바디', type: UserUpdateNameDto })
  @ApiResponse({ status: 200, description: '사용자 이름이 성공적으로 변경되었습니다.', example: { message: "사용자 이름이 성공적으로 변경되었습니다.", newName: '홍길동' } })
  @ApiResponse({ status: 400, description: '잘못된 요청입니다.' })
  async updateName(@Req() req, @Body() updateNameDto: UserUpdateNameDto): Promise<{ message: string, newName: string }> {
    return this.usersService.updateName(req, updateNameDto);
  }


  @Patch('update-password')
  @UseGuards(AuthGuard('jwt'))
  @ApiBearerAuth()
  @ApiOperation({ summary: '비밀번호 변경', description: '로그인한 사용자의 비밀번호를 변경합니다.' })
  @ApiBody({ description: '사용자 비밀번호 변경 바디', type: UserUpdatePasswordDto })
  @ApiResponse({status: 200, description: '비밀번호가 성공적으로 변경되었습니다.', example: {"message": "비밀번호가 성공적으로 변경되었습니다."}})
  @ApiResponse({ status: 400, description: '잘못된 요청입니다.' })
  async updatePassword(@Req() req, @Body() updatePasswordDto: UserUpdatePasswordDto): Promise<{message:string}> {
    return this.usersService.updatePassword(req, updatePasswordDto);
  }


}
