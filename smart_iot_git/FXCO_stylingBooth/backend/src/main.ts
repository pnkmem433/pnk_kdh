import { NestFactory } from '@nestjs/core';
import { AppModule } from './app.module';
import { SwaggerModule } from '@nestjs/swagger';
import { DocumentBuilder } from '@nestjs/swagger';
import { json, urlencoded } from 'express';
import { ValidationPipe } from '@nestjs/common';
import { join } from 'path';
import { NestExpressApplication } from '@nestjs/platform-express';

async function bootstrap() {
  const app = await NestFactory.create<NestExpressApplication>(AppModule);

  const config = new DocumentBuilder()
    .setTitle('ProjFXCO API')
    .setDescription('ProjFXCO API 문서입니다.')
    .setVersion('1.0')
    .build();

  const document = SwaggerModule.createDocument(app, config);
  SwaggerModule.setup('api', app, document);

  app.use(json({ limit: '50mb' }));
  app.use(urlencoded({ extended: true, limit: '50mb' }));

  app.useStaticAssets(join(__dirname, '..', 'public'));

  app.enableCors()

  app.useGlobalPipes(new ValidationPipe({
    transform:true
  }))

  // process.on('unhandledRejection', (reason, promise) => {
  //   console.error('Unhandled Rejection at:', promise, 'reason:', reason);
  //   // 여기에서 추가적인 로깅이나 예외 처리를 수행할 수 있습니다.
  // });
  app.listen(3000, '0.0.0.0', () => {
    console.log('Server running at http://0.0.0.0:3000');
  });
}
bootstrap();