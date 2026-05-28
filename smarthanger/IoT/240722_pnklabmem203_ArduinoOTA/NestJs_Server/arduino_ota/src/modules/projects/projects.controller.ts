import { AuthGuard } from '@nestjs/passport';
import { Controller, Post, Body, Req, UseGuards, Get, Put } from '@nestjs/common';
import { ApiTags, ApiBody, ApiBearerAuth, ApiOperation, ApiResponse, ApiOkResponse } from '@nestjs/swagger';


import { ProjectsService } from './projects.service';

import { ProjectCreateDto } from '../../dto/project/create.dto';
import { ProjectUpdateNameDto } from '../../dto/project/update-name.dto';

@ApiTags('projects')
@Controller('projects')
export class ProjectsController {
  constructor(private readonly projectsService: ProjectsService) { }

  @Get('list')
  @UseGuards(AuthGuard('jwt'))
  @ApiBearerAuth()
  @ApiOperation({ summary: '사용자가 생성한 프로젝트 목록 조회' })
  @ApiOkResponse({ description: '사용자가 생성한 프로젝트 목록 반환', example: [{ "id": 1, "uuid": "프로젝트 uuid", "name": "프로젝트 이름" }] })
  @ApiResponse({ status: 401, description: '권한이 없는 사용자' })
  async getUserProjects(@Req() req) {
    return await this.projectsService.getUserProjects(req.user);
  }


  @Post('create')
  @UseGuards(AuthGuard('jwt'))
  @ApiBearerAuth()
  @ApiOperation({ summary: '새로운 프로젝트 생성' })
  @ApiBody({ description: '프로젝트 생성 바디', type: ProjectCreateDto })
  @ApiOkResponse({ description: '프로젝트가 성공적으로 생성됨', example: { "id": 1, "uuid": "550e8400-e29b-41d4-a716-446655440000", "name": "인공지능 연구 프로젝트" } })
  @ApiResponse({ status: 400, description: '잘못된 요청' })
  @ApiResponse({ status: 409, description: '이미 존재하는 프로젝트 이름입니다.' })
  async create(@Req() req: any, @Body() createProject: ProjectCreateDto): Promise<{ id: number, name: string, uuid: string }> {
    return await this.projectsService.create(req.user, createProject);

  }

  @Put('update-name')
  @UseGuards(AuthGuard('jwt'))
  @ApiBearerAuth()
  @ApiOperation({ summary: '프로젝트 이름 수정' })
  @ApiBody({ description: '프로젝트 이름변경 바디', type: ProjectUpdateNameDto })
  @ApiOkResponse({ description: '프로젝트 이름이 성공적으로 수정됨', example: { "id": 1, "uuid": "550e8400-e29b-41d4-a716-446655440000", "name": "새로운 프로젝트 이름" } })
  @ApiResponse({ status: 404, description: '프로젝트를 찾을 수 없음' })
  @ApiResponse({ status: 409, description: '이미 존재하는 프로젝트 이름입니다.' })
  async updateName(@Req() req: any, @Body() renameProject: ProjectUpdateNameDto) {
    return await this.projectsService.updateName(req.user, renameProject);
  }
}
