import {withService}               from "@use-pico/server";
import {type IConstructionService} from "../api/IConstructionService";

export const withConstructionService = withService<IConstructionService>("@derivean/building/ConstructionService");
