import {Translation}           from "@use-pico/i18n";
import {
    type FilterSchema,
    type OrderBySchema,
    type QuerySchema,
    useQueryEx
}                              from "@use-pico/query";
import {
    isPartial,
    type PicoSchema,
    type WithIdentitySchema
}                              from "@use-pico/schema";
import {type ISelectionStore}  from "@use-pico/selection";
import {type IWithSourceQuery} from "@use-pico/source";
import {StoreProvider}         from "@use-pico/store";
import {
    Alert,
    Divider,
    Group,
    Modal,
    ModalStoreProvider,
    Text
}                              from "@use-pico/ui";
import {type FC}               from "react";
import {useController}         from "react-hook-form";
import type {ValuesSchema}     from "../schema/ValuesSchema";
import {InputEx}               from "./InputEx";
import {CancelButton}          from "./QueryInput/CancelButton";
import {ClearButton}           from "./QueryInput/ClearButton";
import {CommitButton}          from "./QueryInput/CommitButton";
import {WithItem}              from "./QueryInput/WithItem";

export namespace QueryInput {
    export interface Props<
        TValuesSchema extends ValuesSchema,
        TResponseSchema extends WithIdentitySchema,
        TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
    > extends InputEx.Props<TValuesSchema> {
        /**
         * Query used to fetch current entity.
         */
        withSourceQuery: IWithSourceQuery<TQuerySchema, TResponseSchema>;
        /**
         * Store used to manage selection of current entity.
         */
        SelectionStore: ISelectionStore<PicoSchema.Output<TResponseSchema>>;
        /**
         * Component used to render a list of items to select from.
         */
        Selector: Selector<TResponseSchema>;
        /**
         * Render selected item.
         */
        Item: WithItem.Item<TResponseSchema>;
        /**
         * Optional method used to generate filter to fetch an entity (if more complex filter is needed); defaults to an ID.
         */
        toFilter?: (value: string) => PicoSchema.Output<TQuerySchema["shape"]["filter"]>;
        toOrderBy?: () => PicoSchema.Output<TQuerySchema["shape"]["orderBy"]> | undefined;
        onCommit?: CommitButton.Props<TValuesSchema, TResponseSchema>["onCommit"];
    }

    export type Selector<TResponseSchema extends WithIdentitySchema> = FC<SelectorProps<TResponseSchema>>;

    export interface SelectorProps<TResponseSchema extends WithIdentitySchema> {
        /**
         * Access to currently selected item
         */
        SelectionStore: ISelectionStore<PicoSchema.Output<TResponseSchema>>;
    }
}

export const QueryInput = <
    TValuesSchema extends ValuesSchema,
    TResponseSchema extends WithIdentitySchema,
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
>(
    {
        withControl,
        schema,
        withSourceQuery,
        SelectionStore,
        toFilter = id => ({id}),
        toOrderBy = () => undefined,
        Selector,
        Item,
        onCommit,
        ...props
    }: QueryInput.Props<TValuesSchema, TResponseSchema, TQuerySchema>
) => {
    const {
        field: {
                   value,
               },
    } = useController(withControl);

    const result = useQueryEx({
        withQuery: withSourceQuery,
        request:   {
            filter:  value ? toFilter(value) : {id: null},
            orderBy: toOrderBy(),
        },
    });

    return result.isLoading ? <InputEx
        withControl={withControl}
        schema={schema}
        isLoading
        {...props}
    /> : <StoreProvider
        store={SelectionStore}
        values={{
            item:      result.data?.[0],
            selection: result.data?.[0],
        }}
    >
        <ModalStoreProvider>
            <Modal
                modalId={"query-input"}
                modalProps={{
                    size: "75%",
                }}
                title={<>
                    <Text
                        fw={"500"}
                        span
                    >
                        <Translation withLabel={`${withControl.name}.selection.label`}/>
                    </Text>
                    {!isPartial(schema, withControl.name) && <Text
                        ml={4}
                        c={"red"}
                        span
                    >*</Text>}
                </>}
            >
                {result.data?.[0] && <>
                    <Alert
                        title={<Translation namespace={"common.selection"} withLabel={"selected-item.label"}/>}
                    >
                        <Item entity={result.data?.[0]}/>
                    </Alert>
                    <Divider mt={"sm"}/>
                </>}
                <Selector
                    SelectionStore={SelectionStore}
                />
                <Divider my={"sm"}/>
                <Group gap={"md"} justify={"apart"} grow>
                    <Group gap={"sm"}>
                        <CancelButton
                            SelectionStore={SelectionStore}
                        />
                        <ClearButton
                            SelectionStore={SelectionStore}
                        />
                    </Group>
                    <CommitButton
                        withControl={withControl}
                        SelectionStore={SelectionStore}
                        onCommit={onCommit}
                    />
                </Group>
            </Modal>
            <WithItem
                isLoading={result.isFetching}
                withControl={withControl}
                schema={schema}
                Item={Item}
                SelectionStore={SelectionStore}
                {...props}
            />
        </ModalStoreProvider>
    </StoreProvider>;
};
