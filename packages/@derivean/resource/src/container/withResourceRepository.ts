import {withService}              from "@use-pico/container";
import {type IResourceRepository} from "../api/IResourceRepository";

export const withResourceRepository = withService<IResourceRepository>("@derivean/resource/Repository");
