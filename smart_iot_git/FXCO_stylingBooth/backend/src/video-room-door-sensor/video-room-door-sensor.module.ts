import { Module } from '@nestjs/common';
import { VideoRoomDoorSensorController } from './video-room-door-sensor.controller';
import { VideoRoomDoorSensorService } from './video-room-door-sensor.service';
import { TypeOrmModule } from '@nestjs/typeorm';
import { VideoRoomDoorSensor } from 'src/entities/videoRoomDoorSensor.entity';

@Module({
  imports: [TypeOrmModule.forFeature([VideoRoomDoorSensor])],
  controllers: [VideoRoomDoorSensorController],
  providers: [VideoRoomDoorSensorService]
})
export class VideoRoomDoorSensorModule {}
