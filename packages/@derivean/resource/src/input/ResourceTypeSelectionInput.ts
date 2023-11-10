import {
    type ValuesSchema,
    withSourceSelectionInput
}                                   from "@use-pico/form";
import {ResourceTypeQueryStore}     from "../query/type/ResourceTypeQueryStore";
import {withResourceTypeQuery}      from "../query/type/withResourceTypeQuery";
import {ResourceTypeSelectionStore} from "../store/ResourceTypeSelectionStore";

export const ResourceTypeSelectionInput = withSourceSelectionInput({
    withQueryStore:  ResourceTypeQueryStore,
    withSourceQuery: withResourceTypeQuery,
    SelectionStore:  ResourceTypeSelectionStore,
});

export interface ResourceTypeSelectionInput<
    TValuesSchema extends ValuesSchema,
> {
    single: typeof ResourceTypeSelectionInput.single<TValuesSchema>;
    multi: typeof ResourceTypeSelectionInput.multi<TValuesSchema>;
}
