import { Controller, Get, Post, Body, Param, Patch, Delete } from '@nestjs/common';
import { FittingRoomDoorSensorService } from './fitting-room-door-sensor.service';
import { FittingRoomDoorSensor } from 'src/entities/fittingRoomDoorSensor.entity';
import { ApiOperation, ApiTags, ApiParam, ApiBody } from '@nestjs/swagger';

@Controller('fitting-room-door-sensor')
@ApiTags('피팅룸 도어 센서 API')
export class FittingRoomDoorSensorController {
    constructor(private readonly fittingRoomDoorSensorService: FittingRoomDoorSensorService) { }

    @ApiOperation({ summary: '피팅룸 도어 센서 기록 생성' })
    @Post()
    @ApiBody({
        description: '피팅룸 도어센서 생성',
        examples: {
            기본예시: {
                value: {
                    session: 1,
                    is_opened: 0
                }
            }
        }
    })
    async create(@Body() data: Partial<FittingRoomDoorSensor>) {
        return this.fittingRoomDoorSensorService.create(data);
    }

    @ApiOperation({ summary: '피팅룸 도어 센서 조회' })
    @Get()
    async findAll() {
        return this.fittingRoomDoorSensorService.findAll();
    }

    @ApiOperation({ summary: '피팅룸 도어 센서 상세 조회' })
    @Get(':seq')
    @ApiParam({ name: 'seq', type: Number, example: 1, description: '피팅룸 도어센서 고유번호' })
    async findOne(@Param('seq') seq: number) {
        return this.fittingRoomDoorSensorService.findOne(Number(seq));
    }

    @ApiOperation({ summary: '피팅룸 도어 센서 수정' })
    @Patch(':seq')
    @ApiParam({ name: 'seq', type: Number, example: 1, description: '피팅룸 도어센서 고유번호' })
    async update(@Param('seq') seq: number, @Body() data: Partial<FittingRoomDoorSensor>) {
        return this.fittingRoomDoorSensorService.update(Number(seq), data);
    }

    @ApiOperation({ summary: '피팅룸 도어 센서 삭제' })
    @Delete(':seq')
    @ApiParam({ name: 'seq', type: Number, example: 1, description: '피팅룸 도어센서 고유번호' })
    async remove(@Param('seq') seq: number) {
        await this.fittingRoomDoorSensorService.remove(Number(seq));
        return { deleted: true };
    }

    @ApiOperation({ summary: '피팅룸 도어 센서 최근 기록 조회' })
    @Get('last/record/:sessionSeq')
    async getLastRecord(@Param('sessionSeq') sessionSeq: number) {
        return this.fittingRoomDoorSensorService.getLastRecord(sessionSeq);
    }

    @ApiOperation({ summary: '피팅룸 도어 센서 가장 최근 기록 조회' })
    @Get('/latest/one')
    async getAllLastRecord() {
        return this.fittingRoomDoorSensorService.getAllLastRecord();
    }

    // 피팅룸 도어 센서 닫힘 상태 업데이트
    // 0: 열림, 1: 닫힘
    @Post('open/:seq')
    openDoor(@Param('seq') seq: number) {
      return this.fittingRoomDoorSensorService.setDoorOpen(0, seq);
    }

    @Post('close/:seq')
    closeDoor(@Param('seq') seq: number) {
      return this.fittingRoomDoorSensorService.setDoorOpen(1, seq);
    }
}
