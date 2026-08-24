import { Module } from '@nestjs/common';
import { RfidScanController } from './rfid-scan.controller';
import { RfidScanService } from './rfid-scan.service';
import { TypeOrmModule } from '@nestjs/typeorm';
import { RfidScan } from 'src/entities/rfidScan.entity';
import { ClothesTypes } from 'src/entities/clothesTypes.entity';
import { ClothesVideoMatch } from 'src/entities/clothesVideoMatch.entity';
import { VideoContent } from 'src/entities/videoContent.entity';

@Module({
  imports: [TypeOrmModule.forFeature([RfidScan, ClothesTypes, ClothesVideoMatch, VideoContent])],
  controllers: [RfidScanController],
  providers: [RfidScanService]
})
export class RfidScanModule {}
