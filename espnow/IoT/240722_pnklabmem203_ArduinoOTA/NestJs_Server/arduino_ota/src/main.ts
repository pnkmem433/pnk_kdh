import * as dotenv from 'dotenv';
dotenv.config();

import { NestFactory } from '@nestjs/core';
import { AppModule } from './app.module';
import { SwaggerModule, DocumentBuilder } from '@nestjs/swagger';

import { ExpressAdapter } from '@nestjs/platform-express';
import * as express from 'express';
import * as bodyParser from 'body-parser';

async function bootstrap() {
  const server = express();

  // ✅ 임시 허용: 이 경로만 raw text로 받는다 (중요: 정확히 /firmwareDownload)
  server.use(
    '/firmwareDownload',
    bodyParser.text({ type: ['application/json', 'text/plain'] }),
  );

  const app = await NestFactory.create(AppModule, new ExpressAdapter(server));

  app.enableCors({
    origin: ['http://gym907-0001.iptime.org'],
    methods: 'GET,HEAD,PUT,PATCH,POST,DELETE,OPTIONS',
    credentials: true,
  });

  const options = new DocumentBuilder()
    .setTitle('NestJS API')
    .setDescription('The NestJS API description')
    .setVersion('1.0')
    .addBearerAuth()
    .build();

  const document = SwaggerModule.createDocument(app, options);
  SwaggerModule.setup('api', app, document);

  await app.listen(3000, '0.0.0.0');
}
bootstrap();
