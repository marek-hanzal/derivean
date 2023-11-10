import {
    type ValuesSchema,
    withSourceQueryInput
}                               from "@use-pico/form";
import {
    type ComponentProps,
    useCallback
}                               from "react";
import {ProducerInline}         from "../inline/ProducerInline";
import {ProducerQueryStore}     from "../query/ProducerQueryStore";
import {withProducerQuery}      from "../query/withProducerQuery";
import {ProducerSelectionStore} from "../store/ProducerSelectionStore";
import {ProducerTable}          from "../table/ProducerTable";

const ProducerQueryInput = withSourceQueryInput({
    withQueryStore:  ProducerQueryStore,
    withSourceQuery: withProducerQuery,
    SelectionStore: ProducerSelectionStore.single,
});
type ProducerQueryInput<
    TValuesSchema extends ValuesSchema,
> = typeof ProducerQueryInput<TValuesSchema>;

export namespace ProducerSelect {
    export interface Props<
        TValuesSchema extends ValuesSchema,
    > extends Omit<
        ComponentProps<ProducerQueryInput<TValuesSchema>>,
        "Selector" | "Item"
    > {
        tableProps?: Partial<ComponentProps<typeof ProducerTable>>;
    }
}

export const ProducerSelect = <
    TValuesSchema extends ValuesSchema,
>(
    {
        tableProps,
        ...props
    }: ProducerSelect.Props<TValuesSchema>
) => {
    return <ProducerQueryInput
        Selector={useCallback(() => <ProducerTable withLinkLock {...tableProps}/>, [])}
        Item={ProducerInline}
        {...props}
    />;
};
