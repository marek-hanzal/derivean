import {withService}    from "@use-pico/container";
import {type IMigrator} from "../api/IMigrator";

export const withMigrator = withService<IMigrator>("@use-pico/orm/Migrator");
