import { Controller, Post, Delete, Body, UseGuards, UploadedFile, UseInterceptors, Get, Query, Req, NotFoundException } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiResponse, ApiBody, ApiBearerAuth, ApiConsumes, ApiQuery } from '@nestjs/swagger';
import { FileInterceptor } from '@nestjs/platform-express';
import { AuthGuard } from '@nestjs/passport';
import { Express } from 'express';

import { ProjectVersionService } from './versions.service';
import { VersionSearchDto } from 'src/dto/version/search.dto';
import { VersionCreateDto } from 'src/dto/version/create.dto';

@ApiTags('versions')
@Controller('versions')
export class ProjectVersionController {
  constructor(private readonly projectVersionService: ProjectVersionService) { }

  @Post('list')
  @UseGuards(AuthGuard('jwt'))
  @ApiBearerAuth()
  @ApiOperation({ summary: '프로젝트 버전 조회' })
  @ApiBody({ description: '프로젝트 버전 조회 바디', type: VersionSearchDto })
  @ApiResponse({ status: 200, description: '프로젝트 버전 정보 조회 성공', example: [{ id: 1, name: "v1.0.1", created: "2024-09-10T12:34:56Z", isActive: true }] })
  @ApiResponse({ status: 404, description: '프로젝트 버전 정보 없음' })
  async getProjectVersions(@Req() req: any, @Body() versionSearchDto: VersionSearchDto): Promise<{ id: number, name: string, created: Date, isActive: boolean }[]> {
    return await this.projectVersionService.getVersions(req.user, versionSearchDto);
  }

  @Post('create')
  @UseGuards(AuthGuard('jwt'))
  @ApiBearerAuth()
  @UseInterceptors(FileInterceptor('binFile', { dest: './uploads', }))
  @ApiOperation({ summary: '프로젝트 버전 생성' })
  @ApiConsumes('multipart/form-data')
  @ApiBody({ description: '프로젝트 버전 생성 요청 바디', type: VersionCreateDto })
  @ApiResponse({ status: 201, description: '버전이 성공적으로 생성됨', example: { id: 1, name: "v1.0.1", created: "2024-09-10T12:34:56Z", isActive: true } })
  @ApiResponse({ status: 400, description: '잘못된 요청' })
  async createVersion(@Req() req: any, @Body() createVersion: VersionCreateDto, @UploadedFile() binFile: Express.Multer.File) {
    return this.projectVersionService.createVersion(req.user, createVersion, binFile);
  }

  @Delete('delete-latest')
  @UseGuards(AuthGuard('jwt'))
  @ApiBearerAuth()
  @ApiOperation({ summary: '프로젝트의 최신 버전 삭제' })
  @ApiBody({ description: '프로젝트 최신 버전 삭제 요청 바디', type: VersionSearchDto })
  @ApiResponse({description: '최신 버전이 성공적으로 비활성화됨', example: {message: "최신 버전이 성공적으로 비활성화되었습니다."}})
  @ApiResponse({ status: 404, description: '프로젝트 또는 버전을 찾을 수 없음' })
  async deleteLatestVersion(@Body() versionSearchDto: VersionSearchDto, @Req() req: any): Promise<{message: string}>  {
    return this.projectVersionService.deleteLatestVersionByProjectId(req.user, versionSearchDto);
  }
}
