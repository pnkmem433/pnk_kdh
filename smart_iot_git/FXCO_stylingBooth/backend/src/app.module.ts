import { Module } from '@nestjs/common';
import { AppController } from './app.controller';
import { AppService } from './app.service';
import { VideoContentModule } from './video-content/video-content.module';
import { VideoRoomDoorSensorModule } from './video-room-door-sensor/video-room-door-sensor.module';
import { FittingRoomDoorSensorModule } from './fitting-room-door-sensor/fitting-room-door-sensor.module';
import { ClothTypesModule } from './clothes-types/clothes-types.module';
import { ClothesVideoMatchModule } from './clothes-video-match/clothes-video-match.module';
import { RfidScanModule } from './rfid-scan/rfid-scan.module';
import { SessionModule } from './session/session.module';
import { SessionService } from './session/session.service';
import { TypeOrmModule } from '@nestjs/typeorm';
import { ConfigModule } from '@nestjs/config';
import { VideoContent } from './entities/videoContent.entity';
import { VideoRoomDoorSensor } from './entities/videoRoomDoorSensor.entity';
import { FittingRoomDoorSensor } from './entities/fittingRoomDoorSensor.entity';
import { ClothesTypes } from './entities/clothesTypes.entity';
import { ClothesVideoMatch } from './entities/clothesVideoMatch.entity';
import { RfidScan } from './entities/rfidScan.entity';
import { Session } from './entities/session.entity';
import { join } from 'path';
import { ServeStaticModule } from '@nestjs/serve-static';
import { hostname } from 'os';
import { SystemParameterController } from './system-parameter/system-parameter.controller';
import { SystemParameter } from './entities/systemParameter.entity';
import { SystemParameterModule } from './system-parameter/system-parameter.module';
import { PlugModule } from './plug/plug.module';

@Module({
  imports: [
    ConfigModule.forRoot({
      isGlobal: true,
    }),
    ServeStaticModule.forRoot({
      rootPath: join(__dirname, '..', 'public'),
      serveRoot: '/',
      serveStaticOptions: {
        index: 'index.html',
      },
    }),
    TypeOrmModule.forRoot({
      type: 'mysql',
      
      // host: '115.23.192.217',
      // username: 'pnkmem',
      // password: 'pnks1111',
      host: 'host.docker.internal',
      username: 'root',
      password: '71805802',
      port: 3306,
      database: 'projfxco',
      entities: [VideoContent, VideoRoomDoorSensor, FittingRoomDoorSensor, ClothesTypes, ClothesVideoMatch, RfidScan, Session, SystemParameter],
      synchronize: false,
      logging: true,
    }),
    VideoContentModule,
    VideoRoomDoorSensorModule,
    FittingRoomDoorSensorModule,
    ClothTypesModule, ClothesVideoMatchModule,
    RfidScanModule,
    SessionModule,
    SystemParameterModule,
    PlugModule
  ],
  controllers: [AppController],
  providers: [AppService],
})
export class AppModule { }
