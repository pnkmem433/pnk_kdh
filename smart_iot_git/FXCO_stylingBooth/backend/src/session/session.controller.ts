import { Controller, Get, Post, Body, Param, Patch, Delete } from '@nestjs/common';
import { SessionService } from './session.service';
import { Session } from 'src/entities/session.entity';
import { ApiOperation, ApiTags, ApiParam, ApiBody } from '@nestjs/swagger';

@Controller('session')
@ApiTags('세션 API')
export class SessionController {
  constructor(private readonly sessionService: SessionService) {}

  @ApiOperation({ summary: '세션 생성' })
  @Post()
  @ApiBody({
    description: '세션 생성',
    examples: {
      기본예시: {
        value: {
          is_scanned: 0,
          is_video_ended: 0,
          is_activated: 1
        }
      }
    }
  })
  async create(@Body() data: Partial<Session>) {
    return this.sessionService.create(data);
  }

  @ApiOperation({ summary: '세션 조회' })
  @Get()
  async findAll() {
    return this.sessionService.findAll();
  }

  @ApiOperation({ summary: '세션 상세 조회' })
  @Get('detail/:seq')
  @ApiParam({ name: 'seq', type: Number, example: 1, description: '세션 고유번호' })
  async findOne(@Param('seq') seq: number) {
    return this.sessionService.findOneSession(Number(seq));
  }

  @ApiOperation({ summary: '세션 수정' })
  @Patch(':seq')
  @ApiParam({ name: 'seq', type: Number, example: 1, description: '세션 고유번호' })
  async update(@Param('seq') seq: number, @Body() data: Partial<Session>) {
    return this.sessionService.update(Number(seq), data);
  }

  @ApiOperation({ summary: '세션 삭제' })
  @Delete(':seq')
  @ApiParam({ name: 'seq', type: Number, example: 1, description: '세션 고유번호' })
  async remove(@Param('seq') seq: number) {
    await this.sessionService.remove(Number(seq));
    return { deleted: true };
  }

  @ApiOperation({ summary: '마지막 세션 조회' })
  @Get('last')
  async getLastSessions() {
    return this.sessionService.getLastSession();
  }

  @ApiOperation({ summary: '세션이 시작되지 않았는지 체크' })
  @Get('untracked')
  async checkSessionNotStarted() {
    return this.sessionService.checkSessionNotStarted();
  }

  @ApiOperation({ summary: '마지막 세션 pir 값 조회' })
  @Get('last/pir-status')
  async getLastPirStatus() {
    return this.sessionService.getLastPirStatus();
  }



  @ApiOperation({ summary: '마지막 세션 pir 값 업데이트' })
  @Patch('last/pir-status')
  @ApiBody({
    description: 'pir_status 값',
    examples: { 예시: { value: { pir_status: 1 } } }
  })
  async updateLatestSessionPirStatus(@Body('pir_status') pir_status: number) {
    return this.sessionService.updateLatestSessionPirStatus(pir_status);
  }

  @ApiOperation({ summary: 'is_scanned 값 변경' })
  @Patch('is-scanned/:seq')
  @ApiParam({ name: 'seq', type: Number, example: 1, description: '세션 고유번호' })
  @ApiBody({
    description: 'is_scanned 값',
    examples: { 예시: { value: { value: 1 } } }
  })
  async updateIsScanned(
    @Param('seq') seq: number,
    @Body('value') value: number
  ) {
    return this.sessionService.updateIsScanned(Number(seq), value);
  }

  @ApiOperation({ summary: 'is_video_ended 값 변경' })
  @Patch('is-video-ended/:seq')
  @ApiParam({ name: 'seq', type: Number, example: 1, description: '세션 고유번호' })
  @ApiBody({
    description: 'is_video_ended 값',
    examples: { 예시: { value: { value: 1 } } }
  })
  async updateIsVideoEnded(
    @Param('seq') seq: number,
    @Body('value') value: number
  ) {
    return this.sessionService.updateIsVideoEnded(Number(seq), value);
  }

  @ApiOperation({ summary: 'pir_status 값 변경' })
  @Patch('pir-status/:seq')
  @ApiParam({ name: 'seq', type: Number, example: 1, description: '세션 고유번호' })
  @ApiBody({
    description: 'pir_status 값',
    examples: { 예시: { value: { value: 1 } } }
  })
  async updatePirStatus(
    @Param('seq') seq: number,
    @Body('value') value: number
  ) {
    return this.sessionService.updatePirStatus(Number(seq), value);
  }

  @ApiOperation({ summary: '세션활성화여부 조회' })
  @Patch('is-activated/:seq')
  @ApiParam({ name: 'seq', type: Number, example: 1, description: '세션 고유번호' })
  async checkAndDeactivateSession(
    @Param('seq') seq: number
  ) {
    return this.sessionService.checkAndDeactivateSession(Number(seq));
  }

  @ApiOperation({ summary: '문을 닫고 세션 시작 - 세션 상태 인식 및 업데이트를 올인원으로 진행(IOT용)' })
  @ApiBody({
    description: '문을 닫고 세션 시작',
    examples: {
      예시: { value: { pir_status: 1 } }
    }
  })
  @Post('activate-door-close')
  async activateDoorClose(@Body('pir_status') pir_status: number) {
    return this.sessionService.activateDoorClose(pir_status);
  }

  @ApiOperation({ summary: '문을 열고 세션 종료 - 세션 상태 인식 및 업데이트를 올인원으로 진행(IOT용)' })
  @Post('deactivate-door-open')
  async deactivateDoorOpen() {
    return this.sessionService.deactivateDoorOpen();
  }

  @ApiOperation({ summary: '세션 초기화' })
  @Post('emergency-reset')
  async emergencySessionReset() {
    return this.sessionService.emergencySessionReset();
  }

} 
