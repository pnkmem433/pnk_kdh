import { Module } from '@nestjs/common';
import { FittingRoomDoorSensorController } from './fitting-room-door-sensor.controller';
import { FittingRoomDoorSensorService } from './fitting-room-door-sensor.service';
import { TypeOrmModule } from '@nestjs/typeorm';
import { FittingRoomDoorSensor } from 'src/entities/fittingRoomDoorSensor.entity';

@Module({
  imports: [TypeOrmModule.forFeature([FittingRoomDoorSensor])],
  controllers: [FittingRoomDoorSensorController],
  providers: [FittingRoomDoorSensorService]
})
export class FittingRoomDoorSensorModule {}
