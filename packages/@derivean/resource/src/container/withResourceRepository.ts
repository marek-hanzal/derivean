import {withService}              from "@use-pico2/container";
import {type IResourceRepository} from "../api/IResourceRepository";

export const withResourceRepository = withService<IResourceRepository>("@derivean/resource/Repository");
