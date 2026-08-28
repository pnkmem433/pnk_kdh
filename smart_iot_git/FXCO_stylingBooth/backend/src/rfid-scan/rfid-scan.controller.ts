import { Controller, Get, Post, Body, Param, Patch, Delete } from '@nestjs/common';
import { RfidScanService } from './rfid-scan.service';
import { RfidScan } from 'src/entities/rfidScan.entity';
import { ApiOperation, ApiTags, ApiParam, ApiBody } from '@nestjs/swagger';

@Controller('rfid-scan')
@ApiTags('RFID 스캔 API')
export class RfidScanController {
    constructor(private readonly rfidScanService: RfidScanService) { }

    @ApiOperation({ summary: 'RFID 스캔 생성' })
    @Post()
    @ApiBody({
        description: 'RFID 스캔 생성',
        examples: {
            기본예시: {
                value: {
                    session_seq: 1,
                    clothes_product_id: "IT00001",
                    scan_source_type: 0
                }
            }
        }
    })
    async create(@Body() data: Partial<RfidScan>) {
        return this.rfidScanService.create(data);
    }

    @ApiOperation({ summary: 'RFID 스캔 조회' })
    @Get()
    async findAll() {
        return this.rfidScanService.findAll();
    }

    @ApiOperation({ summary: 'RFID 스캔 상세 조회' })
    @Get(':seq')
    @ApiParam({ name: 'seq', type: Number, example: 1, description: 'RFID 스캔 고유번호' })
    async findOne(@Param('seq') seq: number) {
        return this.rfidScanService.findOne(Number(seq));
    }

    @ApiOperation({ summary: 'RFID 스캔 수정' })
    @Patch(':seq')
    @ApiParam({ name: 'seq', type: Number, example: 1, description: 'RFID 스캔 고유번호' })
    async update(@Param('seq') seq: number, @Body() data: Partial<RfidScan>) {
        return this.rfidScanService.update(Number(seq), data);
    }

    @ApiOperation({ summary: 'RFID 스캔 삭제' })
    @Delete(':seq')
    @ApiParam({ name: 'seq', type: Number, example: 1, description: 'RFID 스캔 고유번호' })
    async remove(@Param('seq') seq: number) {
        await this.rfidScanService.remove(Number(seq));
        return { deleted: true };
    }

    @ApiOperation({ summary: 'RFID 스캔 최근 기록 조회' })
    @Get('last/record/:sessionSeq')
    async getLastRecord(@Param('sessionSeq') sessionSeq: number) {
        return this.rfidScanService.getLastRfidScan(sessionSeq);
    }

    @ApiOperation({ summary: 'RFID 스캔 최근 기록 조회 (옷 모든 정보 포함)' })
    @Get('last/record/all/:sessionSeq')
    async getLastRecordWithAllInfo(@Param('sessionSeq') sessionSeq: number) {
        return this.rfidScanService.getLastRfidScanWithAllClothesInfo(sessionSeq);
    }
}   
