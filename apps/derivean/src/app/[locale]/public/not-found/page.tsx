import {tx} from "@use-pico/i18n";
import {
    Page,
    Status
}           from "@use-pico/ui";

export default function Custom404() {
    return <Page>
        <Status
            text={{
                header:  tx()`4o4`,
                title:   tx()`You found a place where nothing is hidden.`,
                message: tx()`Here or there, you see nothing interesting. Try another page.`,
            }}
        />
    </Page>;
};
