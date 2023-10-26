import {type IContainer} from "@use-pico/container";
import {Migrator}        from "../orm/Migrator";
import {Orm}             from "../orm/Orm";
import {withMigrator}    from "./withMigrator";
import {withOrm}         from "./withOrm";

export const withOrmContainer = (container: IContainer.Type) => {
    withMigrator.bind(container, Migrator);
    withOrm.bind(container, Orm);
};
