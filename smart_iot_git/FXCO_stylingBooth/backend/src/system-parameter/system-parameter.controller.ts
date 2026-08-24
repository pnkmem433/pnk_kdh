import { Body, Controller, Get, Param, Patch } from '@nestjs/common';
import { SystemParameterService } from './system-parameter.service';
import { SystemParameter } from 'src/entities/systemParameter.entity';
import { ApiOperation, ApiTags } from '@nestjs/swagger';

@Controller('system-parameter')
@ApiTags('앱 및 기기 인식 속도(파라미터)조정')
export class SystemParameterController {

    constructor(private readonly systemParameterService: SystemParameterService) {}

    @ApiOperation({ summary: '모든 파라미터 조회' })
    @Get('all')
    async findAll(): Promise<SystemParameter[]> {
        return this.systemParameterService.findAll();
    }

    @ApiOperation({ summary: '특정 파라미터 조회' })
    @Get(':seq')
    async findOneBySeq(@Param('seq') seq: number): Promise<SystemParameter> {
        return this.systemParameterService.findOneBySeq(seq);
    }

    @ApiOperation({ summary: '파라미터 수정' })
    @Patch(':seq')
    async paramUpdate(
        @Param('seq') seq: number,
        @Body('parameter') parameter: number
    ): Promise<SystemParameter> {
        return this.systemParameterService.paramUpdate(seq, parameter);
    }
}
