import { Controller, Post, Body } from '@nestjs/common';
import { ApiOperation, ApiTags, ApiResponse } from '@nestjs/swagger';
import { HangerService } from './hanger.service';
import { ConnectHangerDto } from './dto/connect-hanger.dto';
import { PickupDto } from './dto/pickup.dto';
import { PickdownDto } from './dto/pickdown.dto';

@ApiTags('Hanger')
@Controller('hanger')
export class HangerController {
    constructor(private readonly hangerService: HangerService) { }

    @Post('connect')
    @ApiOperation({
        summary: '스마트 행거 서버 연결',
        description: `
스마트 행거가 서버에 최초/재접속할 때 호출합니다.

- 행거 UUID 기준으로 조회
- 행거가 없으면 자동 생성
- 옷이 걸려있으면 옷 정보 반환
- 옷이 없으면 빈 객체 {} 반환
`,
    })
    @ApiResponse({
        status: 200,
        description: '연결 성공 (옷 있음)',
        schema: {
            example: {
                size_color_options: [
                    {
                        size: 55,
                        colors: ['#0060AD', '#000000', '#FFFFFF'],
                    },
                ],
                price: 820000,
                web_site: 'https://www.naver.com',
            },
        },
    })
    @ApiResponse({
        status: 200,
        description: '연결 성공 (옷 없음)',
        schema: {
            example: {},
        },
    })
    connect(@Body() dto: ConnectHangerDto) {
        return this.hangerService.connect(dto.uuid);
    }


   // 📥 픽다운 (NFC 인식)
  @Post('pickdown')
  @ApiOperation({
    summary: '픽다운 (NFC 인식)',
    description: `
행거에 옷이 걸리며 NFC가 인식될 때 호출됩니다.

- 유효한 NFC(hangerleg)만 처리
- 상태를 PICKDOWN으로 전환
- 잘못된 NFC는 자동 무시
`,
  })
  @ApiResponse({
    status: 200,
    description: '픽다운 성공',
    schema: { example: { success: true } },
  })
  @ApiResponse({
    status: 200,
    description: '무시됨 (잘못된 NFC)',
    schema: { example: { ignored: true } },
  })
  pickdown(@Body() dto: PickdownDto) {
    return this.hangerService.pickdown(dto.uuid, dto.pickdownUuid);
  }

  // 📤 픽업 (NFC 해제)
  @Post('pickup')
  @ApiOperation({
    summary: '픽업 (NFC 해제)',
    description: `
행거에서 옷이 제거되어 NFC가 해제될 때 호출됩니다.

- 직전에 유효한 PICKDOWN이 있어야 처리
- 단독 호출 시 자동 무시
`,
  })
  @ApiResponse({
    status: 200,
    description: '픽업 성공',
    schema: { example: { success: true } },
  })
  @ApiResponse({
    status: 200,
    description: '무시됨 (선행 PICKDOWN 없음)',
    schema: {
      example: { ignored: true, reason: 'NO_VALID_PICKDOWN' },
    },
  })
  pickup(@Body() dto: PickupDto) {
    return this.hangerService.pickup(dto.uuid);
  }
}
