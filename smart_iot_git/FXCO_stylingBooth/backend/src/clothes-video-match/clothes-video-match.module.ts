import { Module } from '@nestjs/common';
import { ClothesVideoMatchController } from './clothes-video-match.controller';
import { ClothesVideoMatchService } from './clothes-video-match.service';
import { TypeOrmModule } from '@nestjs/typeorm';
import { ClothesVideoMatch } from 'src/entities/clothesVideoMatch.entity';

@Module({
  imports: [TypeOrmModule.forFeature([ClothesVideoMatch])],
  controllers: [ClothesVideoMatchController],
  providers: [ClothesVideoMatchService]
})
export class ClothesVideoMatchModule {}
