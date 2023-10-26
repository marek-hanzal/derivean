import {withService} from "@use-pico/container";
import {type IOrm}   from "../api/IOrm";

export const withOrm = withService<IOrm>("@use-pico/orm/Orm");
