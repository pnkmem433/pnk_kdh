import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { RackController } from './rack.controller';
import { RackService } from './rack.service';
import { Rack } from '../entities/rack.entity';

@Module({
  imports: [TypeOrmModule.forFeature([Rack])],
  controllers: [RackController],
  providers: [RackService],
  exports: [RackService],
})
export class RackModule {}
