import { Controller, Get } from '@nestjs/common';
import { ApiOperation, ApiResponse, ApiTags } from '@nestjs/swagger';
import { HangerlegService } from './hangerleg.service';

@ApiTags('HangerLeg')
@Controller('hangerleg')
export class HangerlegController {
  constructor(private readonly service: HangerlegService) {}

  @Get('status')
  @ApiOperation({
    summary: '전체 행거랙 위치 상태 조회',
    description:
      '- 모든 hangerleg 위치에 대해 (현재 걸린 행거, 정답 위치)만 반환\n' +
      '- correctHangerLegSeq === null 이면 정상 위치(또는 비어있음/판단불가)\n',
  })
  @ApiResponse({
    status: 200,
    schema: {
      example: {
        items: [
          { hangerLegSeq: 1, hangerSeq: 12, correctHangerLegSeq: null },
          { hangerLegSeq: 2, hangerSeq: 13, correctHangerLegSeq: 5 },
          { hangerLegSeq: 3, hangerSeq: null, correctHangerLegSeq: null },
        ],
      },
    },
  })
  statusAll() {
    return this.service.getAllStatus();
  }
}
