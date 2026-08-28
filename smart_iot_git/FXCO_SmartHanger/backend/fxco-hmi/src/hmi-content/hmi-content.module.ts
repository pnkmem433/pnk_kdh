import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { HmiContentController } from './hmi-content.controller';
import { HmiContentService } from './hmi-content.service';
import { HmiContent } from '../entities/hmi-content.entity';

@Module({
  imports: [TypeOrmModule.forFeature([HmiContent])],
  controllers: [HmiContentController],
  providers: [HmiContentService],
  exports: [HmiContentService],
})
export class HmiContentModule {}


