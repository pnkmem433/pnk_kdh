import { Controller, Get, Post, Body, Param, Patch, Delete } from '@nestjs/common';
import { ClothesVideoMatchService } from './clothes-video-match.service';
import { ClothesVideoMatch } from 'src/entities/clothesVideoMatch.entity';
import { ApiOperation, ApiTags, ApiParam, ApiBody } from '@nestjs/swagger';

@Controller('clothes-video-match')
@ApiTags('옷 비디오 매칭 API')
export class ClothesVideoMatchController {
    constructor(private readonly clothesVideoMatchService: ClothesVideoMatchService) { }

    @ApiOperation({ summary: '옷 비디오 매칭 생성' })
    @Post()
    @ApiBody({
        description: '옷-비디오 매칭 생성',
        examples: {
            기본예시: {
                value: {
                    clothes_seq: 1,
                    video_seq: 2
                }
            }
        }
    })
    async create(@Body() data: Partial<ClothesVideoMatch>) {
        return this.clothesVideoMatchService.create(data);
    }

    @ApiOperation({ summary: '옷 비디오 매칭 조회' })
    @Get()
    async findAll() {
        return this.clothesVideoMatchService.findAll();
    }

    @ApiOperation({ summary: '옷 비디오 매칭 상세 조회' })
    @Get(':seq')
    @ApiParam({ name: 'seq', type: Number, example: 1, description: '옷-비디오 매칭 고유번호' })
    async findOne(@Param('seq') seq: number) {
        return this.clothesVideoMatchService.findOne(Number(seq));
    }

    @ApiOperation({ summary: '옷 비디오 매칭 수정' })
    @Patch(':seq')
    @ApiParam({ name: 'seq', type: Number, example: 1, description: '옷-비디오 매칭 고유번호' })
    async update(@Param('seq') seq: number, @Body() data: Partial<ClothesVideoMatch>) {
        return this.clothesVideoMatchService.update(Number(seq), data);
    }

    @ApiOperation({ summary: '옷 비디오 매칭 삭제' })
    @Delete(':seq')
    @ApiParam({ name: 'seq', type: Number, example: 1, description: '옷-비디오 매칭 고유번호' })
    async remove(@Param('seq') seq: number) {
        await this.clothesVideoMatchService.remove(Number(seq));
        return { deleted: true };
    }

    @ApiOperation({ summary: '옷 비디오 매칭 조회 (상품 ID)' })
    @Get('video/:clothesProductId')
    async findVideoByClothesProductId(@Param('clothesProductId') clothesProductId: string) {
        return this.clothesVideoMatchService.findVideoByClothesProductId(clothesProductId);
    }
}
