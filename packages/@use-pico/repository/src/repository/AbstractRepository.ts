import {type SourceSchema} from "@use-pico/source";
import {type IRepository}  from "../api/IRepository";

export abstract class AbstractRepository<TSourceSchema extends SourceSchema> implements IRepository<TSourceSchema> {
    protected constructor(
        readonly schema: TSourceSchema
    ) {
    }
}
