import { Module } from '@nestjs/common';
import { PlugService } from './plug.service';
import { PlugController } from './plug.controller';

@Module({
  providers: [PlugService],
  controllers: [PlugController]
})
export class PlugModule {}
