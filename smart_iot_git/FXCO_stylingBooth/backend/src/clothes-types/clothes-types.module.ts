import { Module } from '@nestjs/common';
import { ClothesTypesController } from './clothes-types.controller';
import { ClothesTypesService } from './clothes-types.service';
import { TypeOrmModule } from '@nestjs/typeorm';
import { ClothesTypes } from '../entities/clothesTypes.entity';

@Module({
  imports: [TypeOrmModule.forFeature([ClothesTypes])],
  controllers: [ClothesTypesController],
  providers: [ClothesTypesService]
})
export class ClothTypesModule {}
