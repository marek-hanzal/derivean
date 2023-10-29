import {type PrismaClient} from "@prisma/client";
import {withService}       from "@use-pico2/container";

export const withClient = withService<PrismaClient>("@use-pico2/orm/Client");
