import { Controller, Post, Body } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiResponse } from '@nestjs/swagger';
import { ClothesService } from './clothes.service';
import { AttachClothesDto } from './dto/attach-clothes.dto';
import { DetachClothesDto } from './dto/detach-clothes.dto';

@ApiTags('Clothes')
@Controller('clothes')
export class ClothesController {
  constructor(private readonly clothesService: ClothesService) { }

  @Post('attach')
  @ApiOperation({ summary: '행거에 옷 연결' })
  @ApiResponse({
    status: 200,
    schema: {
      example: { success: true },
      description: '옷 연결 성공 시'
    },
  })
  @ApiResponse({
    status: 200,
    schema: {
      example: { ignored: true, reason: 'Hanger not found' },
      description: '옷 연결 무시 시, 이유 포함'
    },
  })
  attach(@Body() dto: AttachClothesDto) {
    return this.clothesService.attachClothes(dto.hangerUuid, dto.clothesTag);
  }

  @Post('detach')
  @ApiOperation({ summary: '행거에서 옷 연결 해제' })
  @ApiResponse({
    status: 200,
    schema: {
      example: { success: true },
      description: '옷 해제 성공 시'
    },
  })
  @ApiResponse({
    status: 200,
    schema: {
      example: { ignored: true, reason: 'No clothes attached to the hanger' },
      description: '옷 해제 무시 시, 이유 포함'
    },
  })
  detach(@Body() dto: DetachClothesDto) {
    return this.clothesService.detachClothes(dto.hangerUuid);
  }

}
