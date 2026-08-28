import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { ClothesController } from './clothes.controller';
import { ClothesService } from './clothes.service';
import { Clothes } from '../entities/clothes.entity';

@Module({
  imports: [TypeOrmModule.forFeature([Clothes])],
  controllers: [ClothesController],
  providers: [ClothesService],
  exports: [ClothesService],
})
export class ClothesModule {}




