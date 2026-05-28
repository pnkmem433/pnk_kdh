import { Controller, Post, Body, Res } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiBody, ApiResponse } from '@nestjs/swagger';
import { Response } from 'express';
import { DownloadService } from './download.service';
import { firmwareDownloadDto } from 'src/dto/firmwareDownload/download.dto';

@ApiTags('firmwareDownload')
@Controller('firmwareDownload')
export class DownloadController {
  constructor(private readonly downloadService: DownloadService) { }

  @Post()
  @ApiOperation({ summary: '펌웨어 다운로드' })
  @ApiBody({description: '펌웨어 다운로드 바디',type: firmwareDownloadDto})
  @ApiResponse({ status: 200, description: '파일 다운로드 링크' })
  @ApiResponse({ status: 400, description: '버전 문제로 인한 에러' })
  async downloadFirmware(@Body() downloadFirmware: firmwareDownloadDto, @Res() res: Response) {
    return res.download((await this.downloadService.downloadFirmware(downloadFirmware)).binFilePath);
  }
}
