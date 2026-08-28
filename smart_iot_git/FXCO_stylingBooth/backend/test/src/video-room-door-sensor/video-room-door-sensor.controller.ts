import { Controller, Get, Post, Body, Param, Patch, Delete } from '@nestjs/common';
import { VideoRoomDoorSensorService } from './video-room-door-sensor.service';
import { VideoRoomDoorSensor } from 'src/entities/videoRoomDoorSensor.entity';
import { ApiOperation, ApiTags, ApiParam, ApiBody } from '@nestjs/swagger';

@Controller('video-room-door-sensor')
@ApiTags('체험관 도어 센서 API')
export class VideoRoomDoorSensorController {
  constructor(private readonly videoRoomDoorSensorService: VideoRoomDoorSensorService) {}

  @ApiOperation({ summary: '체험관 도어 센서 기록 생성' })
  @Post()
  @ApiBody({
    description: '체험관 도어센서 기록 생성',
    examples: {
      기본예시: {
        value: {
          session: 1,
          is_opened: 1  
        }
      }
    }
  })
  async create(@Body() data: Partial<VideoRoomDoorSensor>) {
    return this.videoRoomDoorSensorService.create(data);
  }

  @ApiOperation({ summary: '체험관 도어 센서 기록 조회' })
  @Get()
  async findAll() {
    return this.videoRoomDoorSensorService.findAll();
  }

  @ApiOperation({ summary: '체험관 도어 센서 기록 상세 조회' })
  @Get(':seq')
  @ApiParam({ name: 'seq', type: Number, example: 1, description: '체험관 도어센서 고유번호' })
  async findOne(@Param('seq') seq: number) {
    return this.videoRoomDoorSensorService.findOne(Number(seq));
  }

  @ApiOperation({ summary: '체험관 도어 센서 기록 수정' })
  @Patch(':seq')
  @ApiParam({ name: 'seq', type: Number, example: 1, description: '체험관 도어센서 고유번호' })
  async update(@Param('seq') seq: number, @Body() data: Partial<VideoRoomDoorSensor>) {
    return this.videoRoomDoorSensorService.update(Number(seq), data);
  }

  @ApiOperation({ summary: '체험관 도어 센서 기록 삭제' })
  @Delete(':seq')
  @ApiParam({ name: 'seq', type: Number, example: 1, description: '체험관 도어센서 고유번호' })
  async remove(@Param('seq') seq: number) {
    await this.videoRoomDoorSensorService.remove(Number(seq));
    return { deleted: true };
  }

  @ApiOperation({ summary: '체험관 도어 센서 최근 기록 조회' })
  @Get('last/record/:sessionSeq')
  async getLastRecord(@Param('sessionSeq') sessionSeq: number) {
    return this.videoRoomDoorSensorService.getLastRecord(sessionSeq);
  }


  // 체험관 도어 센서 열림 상태 업데이트
  // 0: 열림, 1: 닫힘
  @Post('open/:seq')
  openDoor(@Param('seq') seq: number) {
    return this.videoRoomDoorSensorService.setDoorOpen(0, seq);
  }

  @Post('close/:seq')
  closeDoor(@Param('seq') seq: number) {
    return this.videoRoomDoorSensorService.setDoorOpen(1, seq);
  }
}
