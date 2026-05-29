import { Controller, Patch, Delete, Body, Req, UseGuards, Post } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiResponse, ApiBody, ApiBearerAuth, ApiQuery, ApiOkResponse } from '@nestjs/swagger';

import { AuthService } from './auth.service';

import { UserLoginDto } from '../../dto/user/login.dto';
import { RefreshTokenDto } from 'src/dto/user/refresh-token.dto';

@ApiTags('auth')
@Controller('auth')
export class AuthController {
  constructor(private readonly authService: AuthService) { }

  @Post('login')
  @ApiOperation({ summary: '로그인 및 JWT 토큰 획득' })
  @ApiBody({ description: '로그인 바디', type: UserLoginDto })
  @ApiOkResponse({ description: '로그인 성공', schema: { example: { message: '로그인이 완료되었습니다.', token: { access_token: 'jwt.access.token' }, profile: { name: 'user Name', } } } })
  @ApiResponse({ status: 401, description: '인증 실패' })
  async login(@Body() userLoginDto: UserLoginDto): Promise<{ message: string, token: { access_token: string }, profile: { name: string } }> {
    return this.authService.login(userLoginDto);
  }

  @Post('refresh')
  @ApiOperation({ summary: '리프레시 토큰으로 새로운 토큰 발급' })
  @ApiBody({ type: RefreshTokenDto })
  @ApiOkResponse({
    schema: {
      example: {
        message: '토큰이 재발급되었습니다.',
        token: { access_token: 'new.access.token', refresh_token: 'new.refresh.token' },
      },
    },
  })
  async refresh(@Body() { refresh_token }: RefreshTokenDto) {
    return this.authService.refresh(refresh_token);
  }

}
