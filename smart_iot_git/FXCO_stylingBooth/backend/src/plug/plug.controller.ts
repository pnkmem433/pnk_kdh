import { Controller, Post, Get, Param, Query } from '@nestjs/common';
import { PlugService, PlugDeviceDTO } from './plug.service';
import { ApiOperation, ApiTags, ApiParam } from '@nestjs/swagger';

@ApiTags('Plug')
@Controller('plug')
export class PlugController {
    constructor(private readonly plugService: PlugService) { }

    // 플러그 상태 조회 - FXCO 전용 (정확한 매칭을 위해 먼저 정의)
    @Get('status-fxco')
    @ApiOperation({ summary: 'FXCO에 간 플러그 상태 조회(id고정:s8e29102b6bb648970azvw)' })
    async getPlugStatusInFxco(): Promise<{ success: boolean; device?: PlugDeviceDTO; message?: string }> {
        try {
            const device = await this.plugService.getPlugStatus('s8e29102b6bb648970azvw');
            if (!device) {
                return {
                    success: false,
                    message: 'FXCO 플러그를 찾을 수 없습니다.'
                };
            }
            return { success: true, device };
        } catch (error) {
            return {
                success: false,
                message: error.message
            };
        }
    }

    // 플러그 상태 조회 - 일반 (파라미터 라우트는 나중에 정의)
    @Get('status/:serialId')
    @ApiOperation({ summary: '시리얼 ID로 플러그 상태 조회' })
    @ApiParam({ name: 'serialId', type: String, description: '플러그 시리얼 ID' })
    async getPlugStatus(
        @Param('serialId') serialId: string,
    ): Promise<{ success: boolean; device?: PlugDeviceDTO; message?: string }> {
        try {
            const device = await this.plugService.getPlugStatus(serialId);
            if (!device) {
                return {
                    success: false,
                    message: `시리얼 ID ${serialId}에 해당하는 플러그를 찾을 수 없습니다.`
                };
            }
            return { success: true, device };
        } catch (error) {
            return {
                success: false,
                message: error.message
            };
        }
    }

    // 플러그 시리얼 ID로 껐다 켜기 (한번에 모든 과정 실행)
    @Post('toggle/:serialId')
    @ApiOperation({ summary: '시리얼 id로 플러그 껐다 켜기' })
    async togglePlugBySerialId(
        @Param('serialId') serialId: string,
    ): Promise<{ success: 
        boolean; message: string; finalState: boolean }> {
        try {
            const result = await this.plugService.togglePlugBySerialId(serialId);
            return result;
        } catch (error) {
            return {
                success: false,
                message: error.message,
                finalState: false
            };
        }
    }
    @Post('toggle-fxco')
    @ApiOperation({ summary: 'FXCO에 간 플러그 껐다 켜기(id고정:s8e29102b6bb648970azvw)' })
    async togglePlugBySerialIdInFxco(
    ): Promise<{ success: boolean; message: string; finalState: boolean }> {
        try {
            const result = await this.plugService.togglePlugBySerialId('s8e29102b6bb648970azvw');
            return result;
        } catch (error) {
            return {
                success: false,
                message: error.message,
                finalState: false
            };
        }
    }

  // 🔌 플러그를 무조건 껐다 켜기 (현재 상태와 관계없이)
  @Post('oneclick/:serialId')
  @ApiOperation({ summary: '플러그 무조건 껐다 켜기 - 원클릭' })
  async forceTogglePlugBySerialId(
    @Param('serialId') serialId: string,
  ): Promise<{ success: boolean; message: string; finalState: boolean }> {
    try {
      const result = await this.plugService.forceTogglePlugBySerialId(serialId);
      return result;
    } catch (error) {
      return { 
        success: false, 
        message: error.message, 
        finalState: false 
      };
    }
  }

  @Post('oneclick-fxco')
  @ApiOperation({ summary: 'FXCO에 간 플러그 무조건 껐다 켜기 - 원클릭(id고정:s8e29102b6bb648970azvw)' })
  async forceTogglePlugBySerialIdInFxco(
  ): Promise<{ success: boolean; message: string; finalState: boolean }> {
    try {
      const result = await this.plugService.forceTogglePlugBySerialId('s8e29102b6bb648970azvw');
      return result;
    } catch (error) {
      return { 
        success: false, 
        message: error.message, 
        finalState: false 
      };
    }
  }

  // 플러그 무조건 켜기
  @Post('on/:serialId')
  @ApiOperation({ summary: '시리얼 ID로 플러그 무조건 켜기' })
  @ApiParam({ name: 'serialId', type: String, description: '플러그 시리얼 ID' })
  async turnOnPlugBySerialId(
    @Param('serialId') serialId: string,
  ): Promise<{ success: boolean; message: string; finalState: boolean }> {
    try {
      const result = await this.plugService.turnOnPlugBySerialId(serialId);
      return result;
    } catch (error) {
      return {
        success: false,
        message: error.message,
        finalState: false
      };
    }
  }

  @Post('on-fxco')
  @ApiOperation({ summary: 'FXCO에 간 플러그 무조건 켜기(id고정:s8e29102b6bb648970azvw)' })
  async turnOnPlugBySerialIdInFxco(): Promise<{ success: boolean; message: string; finalState: boolean }> {
    try {
      const result = await this.plugService.turnOnPlugBySerialId('s8e29102b6bb648970azvw');
      return result;
    } catch (error) {
      return {
        success: false,
        message: error.message,
        finalState: false
      };
    }
  }

  // 플러그 무조건 끄기
  @Post('off/:serialId')
  @ApiOperation({ summary: '시리얼 ID로 플러그 무조건 끄기' })
  @ApiParam({ name: 'serialId', type: String, description: '플러그 시리얼 ID' })
  async turnOffPlugBySerialId(
    @Param('serialId') serialId: string,
  ): Promise<{ success: boolean; message: string; finalState: boolean }> {
    try {
      const result = await this.plugService.turnOffPlugBySerialId(serialId);
      return result;
    } catch (error) {
      return {
        success: false,
        message: error.message,
        finalState: false
      };
    }
  }

    @Post('off-fxco')
    @ApiOperation({ summary: 'FXCO에 간 플러그 무조건 끄기(id고정:s8e29102b6bb648970azvw)' })
    async turnOffPlugBySerialIdInFxco(): Promise<{ success: boolean; message: string; finalState: boolean }> {
      try {
        const result = await this.plugService.turnOffPlugBySerialId('s8e29102b6bb648970azvw');
        return result;
      } catch (error) {
        return {
          success: false,
          message: error.message,
          finalState: false
        };
      }
    }

    // 연구소 플러그 전용 엔드포인트
    @Get('status-lab')
    @ApiOperation({ summary: '연구소 플러그 상태 조회(id고정:s882875773139e319evmqw)' })
    async getPlugStatusInLab(): Promise<{ success: boolean; device?: PlugDeviceDTO; message?: string }> {
        try {
            const device = await this.plugService.getPlugStatus('s882875773139e319evmqw');
            if (!device) {
                return {
                    success: false,
                    message: '연구소 플러그를 찾을 수 없습니다.'
                };
            }
            return { success: true, device };
        } catch (error) {
            return {
                success: false,
                message: error.message
            };
        }
    }

    @Post('on-lab')
    @ApiOperation({ summary: '연구소 플러그 무조건 켜기(id고정:s882875773139e319evmqw)' })
    async turnOnPlugBySerialIdInLab(): Promise<{ success: boolean; message: string; finalState: boolean }> {
      try {
        const result = await this.plugService.turnOnPlugBySerialId('s882875773139e319evmqw');
        return result;
      } catch (error) {
        return {
          success: false,
          message: error.message,
          finalState: false
        };
      }
    }

    @Post('off-lab')
    @ApiOperation({ summary: '연구소 플러그 무조건 끄기(id고정:s882875773139e319evmqw)' })
    async turnOffPlugBySerialIdInLab(): Promise<{ success: boolean; message: string; finalState: boolean }> {
      try {
        const result = await this.plugService.turnOffPlugBySerialId('s882875773139e319evmqw');
        return result;
      } catch (error) {
        return {
          success: false,
          message: error.message,
          finalState: false
        };
      }
    }

}

