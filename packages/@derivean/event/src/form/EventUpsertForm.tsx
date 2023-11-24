import {EventIcon}       from "@derivean/ui";
import {
    BoolInput,
    DateInput,
    NumberInput,
    SelectInput,
    TextInput
}                        from "@use-pico/form";
import {
    t,
    tx
}                        from "@use-pico/i18n";
import {Fieldset}        from "@use-pico/ui";
import {
    type ComponentProps,
    type FC
}                        from "react";
import {EventComponents} from "../ui/EventComponents";

export namespace EventUpsertForm {
    export type Props = Omit<ComponentProps<EventComponents["MutationForm"]>, "inputs" | "defaultValues" | "Render">;
}

export const EventUpsertForm: FC<EventUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <EventComponents.MutationForm
        text={{
            submit:  entity ? t()`Update event (label)` : t()`Create event (label)`,
            success: {
                title:   t()`Success`,
                message: entity ? t()`Event updated` : t()`Event created`,
            }
        }}
        icon={<EventIcon/>}
        inputs={{
            name:     props => <TextInput
                label={t()`Event name`}
                placeholder={tx()`Event name (placeholder)`}
                {...props}
            />,
            type:     props => <SelectInput
                label={t()`Event type`}
                placeholder={tx()`Event type (placeholder)`}
                data={[
                    {
                        label: tx()`Event type [EventInventory]`,
                        value: "EventInventory",
                    },
                    {
                        label: tx()`Event type [EventHero]`,
                        value: "EventHero",
                    },
                ]}
                {...props}
            />,
            from:     props => <DateInput
                text={{
                    label:       t()`Event from (label)`,
                    placeholder: t()`Event from (placeholder)`,
                }}
                {...props}
            />,
            to:       props => <DateInput
                text={{
                    label:       t()`Event to (label)`,
                    placeholder: t()`Event to (placeholder)`,
                }}
                {...props}
            />,
            duration: props => <NumberInput
                label={t()`Event duration`}
                min={0}
                {...props}
            />,
            instant:  props => <BoolInput
                label={t()`Event instant`}
                {...props}
            />,
        }}
        values={entity}
        toRequest={values => (entity ? {
            update: {
                update: values,
                query:  {
                    where: {
                        id: entity.id,
                    },
                },
            },
        } : {
            create: values,
        })}
        defaultValues={{
            name:     "",
            type:     "",
            duration: 0,
            instant:  false,
            from:     "",
            to:       "",
            userId:   "",
        }}
        Render={({Input}) => <>
            <Fieldset
                legend={t()`Event (fieldset)`}
            >
                <Input name={"name"}/>
                <Input name={"type"}/>
                <Input name={"instant"}/>
            </Fieldset>
            <Fieldset
                legend={t()`Event duration (fieldset)`}
            >
                <Input name={"duration"}/>
                <Input name={"from"}/>
                <Input name={"to"}/>
            </Fieldset>
        </>}
        {...props}
    />;
};
