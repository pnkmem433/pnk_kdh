import { Body, Controller, Post } from '@nestjs/common';
import { ApiOperation, ApiResponse, ApiTags } from '@nestjs/swagger';
import { TimeTrackingDto } from './dto/time-tracking.dto';
import { TimeTrackingService } from './time-tracking.service';

@ApiTags('TimeTracking')
@Controller('time-tracking')
export class TimeTrackingController {
  constructor(private readonly service: TimeTrackingService) {}

  @Post()
  @ApiOperation({
    summary: '작업 시간 측정 기록',
    description: 'uuid로 행거를 조회하여 시작/종료 시각을 time_check에 저장',
  })
  @ApiResponse({ status: 200, schema: { example: { success: true } } })
  @ApiResponse({ status: 200, schema: { example: { ignored: true, reason: 'HANGER_NOT_FOUND' } } })
  create(@Body() dto: TimeTrackingDto) {
    return this.service.create(dto);
  }
}
