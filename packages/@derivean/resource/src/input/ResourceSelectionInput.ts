import {
    type ValuesSchema,
    withSourceSelectionInput
}                               from "@use-pico/form";
import {ResourceQueryStore}     from "../query/ResourceQueryStore";
import {withResourceQuery}      from "../query/withResourceQuery";
import {ResourceSelectionStore} from "../store/ResourceSelectionStore";

export const ResourceSelectionInput = withSourceSelectionInput({
    withQueryStore:  ResourceQueryStore,
    withSourceQuery: withResourceQuery,
    SelectionStore:  ResourceSelectionStore,
});

export interface ResourceSelectionInput<
    TValuesSchema extends ValuesSchema,
> {
    single: typeof ResourceSelectionInput.single<TValuesSchema>;
    multi: typeof ResourceSelectionInput.multi<TValuesSchema>;
}
