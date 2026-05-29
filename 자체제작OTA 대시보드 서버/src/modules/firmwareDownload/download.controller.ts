import { Body, Controller, Get, Post, Query, Res } from '@nestjs/common';
import { ApiBody, ApiOperation, ApiResponse, ApiTags } from '@nestjs/swagger';
import { Response } from 'express';
import * as path from 'path';
import { firmwareDownloadDto } from 'src/dto/firmwareDownload/download.dto';
import { DownloadService } from './download.service';
import { buildOtaLogContext, otaError, otaLog, parseFirmwareDownloadInput } from './ota-log.util';

@ApiTags('firmwareDownload')
@Controller('firmwareDownload')
export class DownloadController {
  constructor(private readonly downloadService: DownloadService) {}

  private currentFirmwareFamilyOf(downloadFirmware: firmwareDownloadDto | string) {
    if (typeof downloadFirmware !== 'string') {
      return String(downloadFirmware.currentFirmwareFamily ?? '').trim().toLowerCase();
    }

    try {
      const parsed = JSON.parse(downloadFirmware.replace(/\t/g, '').trim()) as {
        currentFirmwareFamily?: unknown;
      };
      return String(parsed.currentFirmwareFamily ?? '').trim().toLowerCase();
    } catch {
      return '';
    }
  }

  @Post()
  @ApiOperation({ summary: '펌웨어 다운로드' })
  @ApiBody({ description: '펌웨어 다운로드 요청 바디', type: firmwareDownloadDto })
  @ApiResponse({ status: 201, description: '업데이트 BIN 파일 다운로드' })
  @ApiResponse({ status: 400, description: '현재 버전 기준으로 업데이트가 없거나 잘못된 요청' })
  @ApiResponse({ status: 404, description: '프로젝트 또는 BIN 파일을 찾을 수 없음' })
  async downloadFirmware(@Body() downloadFirmware: firmwareDownloadDto | string, @Res() res: Response) {
    const parsedInput = parseFirmwareDownloadInput(downloadFirmware);
    const context = buildOtaLogContext(
      parsedInput,
      res.req.headers as Record<string, string | string[] | undefined>,
    );
    const result = await this.downloadService.downloadFirmware(downloadFirmware);
    const currentFamily = this.currentFirmwareFamilyOf(downloadFirmware);
    const targetFamily = String(result.firmwareFamily ?? '').trim().toLowerCase();
    const fileName = path.basename(result.binFilePath);

    otaLog(
      { ...context, currentFirmwareFamily: currentFamily || context.currentFirmwareFamily },
      `응답 준비 완료: 현재계열=${currentFamily || '미확인'}, 대상계열=${targetFamily || '미확인'}`,
    );
    otaLog(context, `파일 전송 시작: ${fileName}`);
    otaLog(context, '서버가 펌웨어 파일 응답을 시작합니다. 실제 다운로드/플래시 진행은 플러그 시리얼 로그를 확인하세요.');

    res.setHeader('X-Target-Firmware-Family', targetFamily);

    return res.status(201).download(result.binFilePath, fileName, (error) => {
      if (error) {
        otaError(context, `파일 전송 실패: ${fileName}`, error);
        return;
      }
      otaLog(context, `파일 전송 완료: ${fileName}`);
      otaLog(context, '서버 기준 작업은 끝났습니다. 이제 플러그가 파일 검증과 플래시 쓰기 단계를 진행합니다.');
    });
  }

  @Get('migration-safeboot')
  async downloadMigrationSafeboot(@Query('chipType') chipType: string, @Res() res: Response) {
    const context = { chipType: String(chipType ?? '').trim().toLowerCase() };
    otaLog(context, '마이그레이션 파티션 브리지 요청');
    const result = await this.downloadService.downloadMigrationSafeboot(chipType);
    otaLog(context, `마이그레이션 파티션 브리지 응답: ${result.binFilePath}`);
    return res.download(result.binFilePath);
  }

  @Get('migration-real-safeboot')
  async downloadRealSafeboot(@Query('chipType') chipType: string, @Res() res: Response) {
    const context = { chipType: String(chipType ?? '').trim().toLowerCase() };
    otaLog(context, '마이그레이션 실사이즈 세이프부트 요청');
    const result = await this.downloadService.downloadRealSafeboot(chipType);
    otaLog(context, `마이그레이션 실사이즈 세이프부트 응답: ${result.binFilePath}`);
    return res.download(result.binFilePath);
  }

  @Get('migration-main')
  async downloadMigrationMain(
    @Query('projectId') projectId: string,
    @Query('chipType') chipType: string,
    @Res() res: Response,
  ) {
    const context = {
      projectId: Number(projectId),
      chipType: String(chipType ?? '').trim().toLowerCase(),
    };
    otaLog(context, '마이그레이션 메인 펌웨어 요청');
    const result = await this.downloadService.downloadMigrationMain(Number(projectId), chipType);
    otaLog(context, `마이그레이션 메인 펌웨어 응답: ${result.binFilePath}`);
    return res.download(result.binFilePath);
  }
}
