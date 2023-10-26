import {withService}  from "@use-pico/container";
import {type IConfig} from "../api/IConfig";

export const withConfig = withService<IConfig>("@use-pico/orm/Config");
