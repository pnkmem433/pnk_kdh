import { Controller, Get, Post, Body, HttpCode, HttpStatus, UnauthorizedException } from '@nestjs/common';
import { AppService } from './app.service';
import { ApiOperation, ApiTags, ApiBody } from '@nestjs/swagger';

@ApiTags('인증')
@Controller()
export class AppController {
  constructor(private readonly appService: AppService) {}

  @Get('hello')
  getHello(): string {
    return this.appService.getHello();
  }

  @ApiOperation({ summary: '대시보드 비밀번호 검증' })
  @ApiBody({
    schema: {
      type: 'object',
      properties: {
        password: {
          type: 'string',
          description: '대시보드 접근 비밀번호'
        }
      },
      required: ['password']
    }
  })
  @Post('auth/dashboard')
  @HttpCode(HttpStatus.OK)
  verifyDashboardPassword(@Body() body: { password: string }) {
    if (!body.password) {
      throw new UnauthorizedException('비밀번호가 필요합니다.');
    }

    const isValid = this.appService.verifyDashboardPassword(body.password);
    
    if (isValid) {
      return {
        success: true,
        message: '인증 성공'
      };
    } else {
      throw new UnauthorizedException('비밀번호가 올바르지 않습니다.');
    }
  }
}
