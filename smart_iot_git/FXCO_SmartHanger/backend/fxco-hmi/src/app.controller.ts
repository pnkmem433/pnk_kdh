import { Controller, Get } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiResponse } from '@nestjs/swagger';

@ApiTags('health')
@Controller()
export class AppController {
  @Get()
  @ApiOperation({ summary: '서버 상태 확인', description: '서버가 정상적으로 실행 중인지 확인합니다.' })
  @ApiResponse({ status: 200, description: '서버 정상 작동' })
  getHello(): string {
    return 'FXCO HMI 서버가 정상적으로 실행 중입니다.';
  }

  @Get('health')
  @ApiOperation({ summary: '헬스 체크', description: '서버의 상태 정보를 반환합니다.' })
  @ApiResponse({ 
    status: 200, 
    description: '서버 상태 정상',
    schema: {
      type: 'object',
      properties: {
        status: { type: 'string', example: 'ok' },
        message: { type: 'string', example: 'FXCO HMI 서버 상태 정상' },
        timestamp: { type: 'string', example: '2024-12-22T00:00:00.000Z' }
      }
    }
  })
  getHealth() {
    return {
      status: 'ok',
      message: 'FXCO HMI 서버 상태 정상',
      timestamp: new Date().toISOString(),
    };
  }
}

