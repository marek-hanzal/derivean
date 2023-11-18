import {type Connection} from "../../api/Connection";
import {type Database}   from "../../api/Database";
import {withService}     from "../../service/withService";

export const withConnection = withService<Connection<Database>>("@use-pico/server/Connection");
export type withConnection = typeof withConnection["service"];
